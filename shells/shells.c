/**************************************************************************************************
 * Shells module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef SHELLS



#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */
#include <string.h>

#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <pty.h>
#include <sys/stat.h>  /* mkdir */
#include <sys/types.h> /* mkdir */

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* WNS */
#include "../base64/base64.h"         /* encode decode */
#include "../wtalk.h"                 /* RUNNING_PROJECTS_PATH */
#include "../wmsgpack/wmsgpack.h"     /* msgpack handling */
#include "shells.h"                   /* shells module api */
#include "shells_helper.h"            /* read routine */
#include "wtalk_config.h"


/* Variables from wtalk.c */
extern const char *build_file_str; /* build_file_str */
extern const char *board_str;      /* board name */
extern const char *jid_str;        /* jid */
extern const char *owner_str;      /* owner_str */
extern const char *sudo_str;       /* sudo command from wtalk.c */
extern const char *shell_cmd;      /* start shell command from wtalk.c */

extern xmpp_ctx_t *ctx;   /* XMPP context    */
extern xmpp_conn_t *conn; /* XMPP connection */

shell_t *shells_vector[MAX_SHELLS]; /* All shells */

pthread_mutex_t shells_lock; /* shells mutex */

extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);

static char **string_to_array(char *str, int *size) {
  char ** res  = NULL;
  char *  p    = strtok (str, " ");
  int n_spaces = 0;

  while (p) {
    res = realloc (res, sizeof (char*) * ++n_spaces);

    if (res == NULL)
      exit (-1); /* memory allocation failed */

    res[n_spaces-1] = p;

    p = strtok (NULL, " ");
  }


  n_spaces++;
  res = realloc (res, sizeof (char*) * n_spaces);
  res[n_spaces-1] = NULL;

  *size = n_spaces;
  return res;
}

// static bool get_open_attributes(xmpp_stanza_t *stanza, char **request_attr, char **width_attr,
//   char **height_attr, char **projectid_attr, char **userid_attr)
// {
//   *request_attr   = xmpp_stanza_get_attribute(stanza, "request");
//   *width_attr     = xmpp_stanza_get_attribute(stanza, "width");
//   *height_attr    = xmpp_stanza_get_attribute(stanza, "height");
//   *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid");
//   *userid_attr    = xmpp_stanza_get_attribute(stanza, "userid");

//   if (*request_attr == NULL || *width_attr == NULL || *height_attr == NULL) {
//     werr("No request, width or height attribute in shells open");
//     return false;
//   }

//   if (*projectid_attr != NULL && *userid_attr == NULL) {
//     werr("No userid attribute but projectid attribute is provided");
//     return false;
//   }

//   return true;
// }

static int get_entry_in_shells_vector() {
  int shell_index;

  pthread_mutex_lock(&shells_lock);
  for (shell_index = 0; shell_index < MAX_SHELLS; shell_index++) {
    if (shells_vector[shell_index] == NULL) {
      break;
    }
  }
  pthread_mutex_unlock(&shells_lock);

  return shell_index;
}

static char **concatenation_of_local_and_user_env(char **local_env, int local_env_size) {
  /* Get size of user environmet variables */
  int environ_size = 0;
  while(environ[environ_size]) {
    environ_size++;
  }

  /* Concatenate local and */
  char **all_env = malloc((environ_size + local_env_size) * sizeof(char *));
  memcpy(all_env, environ, environ_size * sizeof(char *));
  memcpy(all_env + environ_size, local_env, local_env_size * sizeof(char *));

  return all_env;
}

static bool allocate_memory_for_new_shell(int shell_index, int pid, int fdm,
  long int width, long int height,
  char *request_attr, char *projectid_attr, char *userid_attr)
{
  pthread_mutex_lock(&shells_lock);
  shells_vector[shell_index] = malloc(sizeof(shell_t));
  if (shells_vector[shell_index] == NULL) {
    werr("malloc failed");
    return false;
  }
  shells_vector[shell_index]->conn           = conn;
  shells_vector[shell_index]->ctx            = ctx;
  shells_vector[shell_index]->id             = shell_index;
  shells_vector[shell_index]->pid            = pid;
  shells_vector[shell_index]->fdm            = fdm;
  shells_vector[shell_index]->width          = width;
  shells_vector[shell_index]->height         = height;
  if (request_attr != NULL) {
    shells_vector[shell_index]->request_attr = strdup(request_attr);
  } else {
    shells_vector[shell_index]->request_attr = NULL;
  }
  if (projectid_attr != NULL) {
    shells_vector[shell_index]->projectid    = strdup(projectid_attr);
  } else {
    shells_vector[shell_index]->projectid    = NULL;
  }
  if (userid_attr != NULL) {
    shells_vector[shell_index]->userid       = strdup(userid_attr);
  } else {
    shells_vector[shell_index]->userid       = NULL;
  }
  shells_vector[shell_index]->close_request = -1;
  pthread_mutex_unlock(&shells_lock);

  return true;
}

static void open_normal_shell(char *request_attr, char *width_attr, char *height_attr) {
  /* Get an entry in the shells_vector */
  int shell_index = get_entry_in_shells_vector();
  if (shell_index == MAX_SHELLS) {
    werr("No shells left");
    goto label_fail;
  }

  /* Get width and height for winsize */
  char *endptr;
  long width = strtol(width_attr, &endptr, 10);
  if (*endptr != '\0') {
    werr("Wrong width attribute: %s", width_attr);
    goto label_fail;
  }
  long height = strtol(height_attr, &endptr, 10);
  if (*endptr != '\0') {
    werr("Wrong height attribute: %s", height_attr);
    goto label_fail;
  }
  struct winsize ws = {height, width, 0, 0};

  /* Fork */
  int fdm;
  int pid = forkpty(&fdm, NULL, NULL, &ws);
  if (pid == -1) { /* Error in forkpty */
    werr("SYSERR forkpty");
    goto label_fail;
  }

  if (pid == 0) { /* Child from forkpty */
    /* Build local environment variables */
    char wyliodrin_board_env[64];
    snprintf(wyliodrin_board_env, 64, "wyliodrin_board=%s", board_str);

    char wyliodrin_server[64];
    snprintf(wyliodrin_server, 64, "wyliodrin_server=%d.%d", WTALK_VERSION_MAJOR,
      WTALK_VERSION_MINOR);

    char *local_env[] = { wyliodrin_board_env, wyliodrin_server, "HOME=/wyliodrin", "TERM=xterm",
      NULL };
    int local_env_size = sizeof(local_env) / sizeof(*local_env);
    char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

    /* Start bash */
    chdir("/wyliodrin");
    int exec_argv_size;
    char **exec_argv = string_to_array((char *)shell_cmd, &exec_argv_size);
    execvpe(exec_argv[0], exec_argv, all_env);

    werr("bash failed");
    exit(EXIT_FAILURE);
  }

  /* Parent from forkpty */
  if (!allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
    request_attr, NULL, NULL))
  {
    goto label_fail;
  }

  /* Create new thread for read routine */
  pthread_t rt;
  int rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[shell_index]);
  if (rc < 0) {
    werr("pthread_create fail");
    goto label_fail;
  }
  pthread_detach(rt);

  /* Send success response */
  send_shells_open_response(request_attr, true, shell_index, false);

  return;

  label_fail:
    send_shells_open_response(request_attr, false, -1, false);
    return;
}



static void open_project_shell(char *request_attr, char *width_attr, char *height_attr,
                               char *projectid_attr, char *userid_attr) {
  /* Get the shell_index in case of a running project */
  int shell_index;
  char projectid_filepath[128];
  snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);
  if (request_attr != NULL) {
    int projectid_fd = open(projectid_filepath, O_RDWR);
    if (projectid_fd != -1) {
      read(projectid_fd, &shell_index, sizeof(int));
      send_shells_open_response(request_attr, true, shell_index, true);
      return;
    }
  }

  /* Get an entry in shells_vector */
  shell_index = get_entry_in_shells_vector();
  if (shell_index == MAX_SHELLS) {
    werr("No shells left");
    goto label_fail;
  }

  /* Get width and height for winsize */
  char *endptr;
  long width  = DEFAULT_WIDTH;
  long height = DEFAULT_HEIGHT;
  if (request_attr != NULL) {
    width = strtol(width_attr, &endptr, 10);
    if (*endptr != '\0') {
      werr("Wrong width attribute: %s", width_attr);
      goto label_fail;
    }
    height = strtol(height_attr, &endptr, 10);
    if (*endptr != '\0') {
      werr("Wrong height attribute: %s", height_attr);
      goto label_fail;
    }
  }
  struct winsize ws = {height, width, 0, 0};

  /* Fork */
  int fdm;
  int pid = forkpty(&fdm, NULL, NULL, &ws);
  if (pid == -1) { /* Error in forkpty */
    werr("SYSERR forkpty");
    goto label_fail;
  }

  if (pid == 0) { /* Child from forkpty */
    /* Write the shells index in the tmp project's file */
    int projectid_fd = open(projectid_filepath, O_CREAT | O_RDWR, 0600);
    if (projectid_fd == -1) {
      werr("Could not open: %s", projectid_filepath);
      return;
    }
    write(projectid_fd, &shell_index, sizeof(int));
    close(projectid_fd);

    char cd_path[256];
    snprintf(cd_path, 256, "%s/%s", build_file_str, projectid_attr);
    if (chdir(cd_path) == -1) {
      werr("Could not chdir in %s", cd_path);
      return;
    }

    /* Build local environment variables */
    char wyliodrin_project_env[64];
    snprintf(wyliodrin_project_env, 64, "wyliodrin_project=%s", projectid_attr);
    char wyliodrin_userid_env[64];
    snprintf(wyliodrin_userid_env, 64, "wyliodrin_userid=%s",
      userid_attr == NULL ? userid_attr : "null");
    char wyliodrin_session_env[64];
    snprintf(wyliodrin_session_env, 64, "wyliodrin_session=%s",
      request_attr == NULL ? request_attr : "null");
    char wyliodrin_board_env[64];
    snprintf(wyliodrin_board_env, 64, "wyliodrin_board=%s", board_str);
    char wyliodrin_jid_env[64];
    snprintf(wyliodrin_jid_env, 64, "wyliodrin_jid=%s", jid_str);
    char wyliodrin_server[64];
    snprintf(wyliodrin_server, 64, "wyliodrin_server=%d.%d", WTALK_VERSION_MAJOR,
      WTALK_VERSION_MINOR);

    #ifdef USEMSGPACK
      char wyliodrin_usemsgpack_env[64];
      snprintf(wyliodrin_usemsgpack_env, 64, "wyliodrin_usemsgpack=1");

      char *local_env[] = { wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
                            wyliodrin_board_env, wyliodrin_jid_env, "HOME=/wyliodrin",
                            "TERM=xterm", wyliodrin_server, wyliodrin_usemsgpack_env, NULL };
    #else
      char *local_env[] = { wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
                            wyliodrin_board_env, wyliodrin_jid_env, "HOME=/wyliodrin",
                            "TERM=xterm", wyliodrin_server, NULL };
    #endif

    int local_env_size = sizeof(local_env) / sizeof(*local_env);
    char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

    char makefile_name[64];
    snprintf(makefile_name, 64, "Makefile.%s", board_str);

    char *exec_argv[] = {"make", "-f", makefile_name, "run", NULL};

    execvpe(exec_argv[0], exec_argv, all_env);

    werr("make failed");
    exit(EXIT_FAILURE);
  }

  /* Parent from forkpty */
  if (!allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
    request_attr, projectid_attr, userid_attr))
  {
    goto label_fail;
  }

  /* Write the project id in RUNNING_PROJECTS_PATH if a project must run */
  if (request_attr != NULL) {
    int open_rc = open(RUNNING_PROJECTS_PATH, O_WRONLY|O_APPEND);
    if (open_rc == -1) {
      werr("Error while trying to open " RUNNING_PROJECTS_PATH);
    } else {
      char projectid_attr_with_colon[strlen(projectid_attr) + 2];
      sprintf(projectid_attr_with_colon, "%s:", projectid_attr);
      int write_rc = write(open_rc, projectid_attr_with_colon,
          strlen(projectid_attr_with_colon));
      werr2(write_rc == -1, "Error while writing to " RUNNING_PROJECTS_PATH);
    }
  }

  /* Create new thread for read routine */
  pthread_t rt;
  int rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[shell_index]);
  if (rc < 0) {
    werr("pthread_create fail");
    goto label_fail;
  }
  pthread_detach(rt);

  /* Send success response */
  if (request_attr != NULL) {
    send_shells_open_response(request_attr, true, shell_index, false);
  }

  return;

  label_fail:
  if (request_attr != NULL) {
    send_shells_open_response(request_attr, false, -1, false);
  }
}

void start_dead_projects() {
  FILE *fp;

  fp = fopen(RUNNING_PROJECTS_PATH, "r");
  if (fp == NULL) {
    werr("Error on open " RUNNING_PROJECTS_PATH);
    return;
  }

  char projectid[128];
  while (fscanf(fp, "%[^:]:", projectid) != EOF) {
    wlog("projectid = %s\n\n\n", projectid);
    if (strlen(projectid) > 0) {
      open_project_shell(NULL, NULL, NULL,
        projectid, NULL);
    }
    sleep(1);
  }

  fclose(fp);
}

void init_shells()
{
  static bool shells_initialized = false;

  mkdir("/tmp/wyliodrin", 0700);

  if (!shells_initialized) {
    int i;

    pthread_mutex_lock(&shells_lock);
    for(i = 0; i < MAX_SHELLS; i++) {
      shells_vector[i] = NULL;
    }
    pthread_mutex_unlock(&shells_lock);

    shells_initialized = true;
  }
}

void shells(const char *from, const char *to, hashmap_p h) {
  if (!SANITY_CHECK(from != NULL, to != NULL, h != NULL)) return;

  char *action = (char *)hashmap_get(h, "a");
  if (action == NULL) {
    werr("There is no action in shells msgpack map");
    return;
  }

  if (strcmp(action, "o") == 0) {
    shells_open(h);
  } else if (strcmp(action, "c") == 0) {
    shells_close(h);
  } else if (strcmp(action, "k") == 0) {
    shells_keys(h);
  } else if (strcmp(action, "p") == 0) {
    shells_poweroff();
  } else {
    werr("Unknown action: %s", action);
  }
}

void shells_open(hashmap_p h) {
  if (!SANITY_CHECK(h != NULL)) goto fail;

  /* Get data */
  char *request    = (char *)hashmap_get(h, "r");
  char *width      = (char *)hashmap_get(h, "w");
  char *height     = (char *)hashmap_get(h, "h");
  char *project_id = (char *)hashmap_get(h, "p");
  char *user_id    = (char *)hashmap_get(h, "u");

  if (!SANITY_CHECK(request != NULL,
                    width   != NULL,
                    height  != NULL)) {
    goto fail;
  }

  if (project_id == NULL) { /* A normal shell must be open */
    open_normal_shell(request, width, height);
  } else { /* A project must be run */
    open_project_shell(request, width, height,
      project_id, user_id);
  }

  return;

  fail:
    send_shells_open_response(request, false, 0, false);
}

void send_shells_open_response(char *request, bool success, int8_t shell_id, bool running) {
  werr("send_shells_open_response");

  int msgpack_map_size;

  char shell_id_str[4];
  if (success) {
    snprintf(shell_id_str, 4, "%d", shell_id);
  }

  char *msgpack_map = build_msgpack_map(&msgpack_map_size,
    "a",  "o",
    "rq", request,
    "rp", success ? "d" : "e",
    "s",  success ? shell_id_str : "",
    "rn", success ? (running ? "t" : "f") : "");

  if (msgpack_map == NULL) {
    werr("build_msgpack_map failed");
    return;
  }

  char *encoded_data = malloc(BASE64_SIZE(msgpack_map_size));
  if (encoded_data == NULL) {
    werr("malloc failed: %s", strerror(errno));
    free(msgpack_map);
    return;
  }
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(msgpack_map_size),
    (const unsigned char *)msgpack_map, msgpack_map_size);
  if (encoded_data == NULL) {
    werr("Could not encode");
    free(msgpack_map);
    return;
  }

  xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", owner_str);
  xmpp_stanza_t *w_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(w_stz, "w");
  xmpp_stanza_set_ns(w_stz, WNS);
  xmpp_stanza_set_attribute(w_stz, "d", encoded_data);
  xmpp_stanza_add_child(message_stz, w_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(w_stz);
  xmpp_stanza_release(message_stz);
}

void shells_close(hashmap_p h) {
  if (!SANITY_CHECK(h != NULL)) goto fail;

  char *endptr; /* strtol endptr */

  /* Get request attribute */
  char *request_attr = (char *)hashmap_get(h, "r"); /* request attribute */
  if(request_attr == NULL) {
    werr("Error while getting request attribute");
    goto fail;
  }
  long int request = strtol(request_attr, &endptr, 10);
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", request_attr, request);
    goto fail;
  }

  /* Get shellid attribute */
  char *shellid_attr = (char *)hashmap_get(h, "s");; /* shellid attribute */
  if(shellid_attr == NULL) {
    werr("Error while getting shellid attribute");
    goto fail;
  }
  long int shellid = strtol(shellid_attr, &endptr, 10); /* shellid value */
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", shellid_attr, shellid);
    goto fail;
  }

  /* Set close request or ignore it if it comes from unopened shell */
  pthread_mutex_lock(&shells_lock);
  if (shells_vector[shellid] != NULL) {
    shells_vector[shellid]->close_request = request;
    close(shells_vector[shellid]->fdm);
  } else {
    pthread_mutex_unlock(&shells_lock);
    goto fail;
  }
  pthread_mutex_unlock(&shells_lock);

  /* Detach from screen session */
  if ((char *)hashmap_get(h, "b") == NULL) { /* Background */
    int pid = fork();

    /* Return if fork failed */
    if (pid == -1) {
      werr("SYSERR fork");
      perror("fork");
      goto fail;
    }

    /* Child from fork */
    if (pid == 0) {
      char screen_quit_cmd[32];
      // sprintf(screen_quit_cmd, "screen -S shell%ld -X quit", shellid);
      snprintf(screen_quit_cmd, 32, "kill -9 %d", shells_vector[shellid]->pid);
      system(screen_quit_cmd);
      waitpid(shells_vector[shellid]->pid, NULL, 0);
      exit(EXIT_SUCCESS);
    }
    waitpid(pid, NULL, 0);
  }

  fail:
    return;
}

void shells_keys(hashmap_p h) {
  char *endptr; /* strtol endptr */

  /* CAREFULL: intead of putting the data in the text section of a stanza,
     the data will be put in the msgpack section, corresponding to the d attribute */
  char *data_str = (char *)hashmap_get(h, "d"); /* data string */
  if(data_str == NULL) {
    wlog("Return from shells_keys due to NULL data");
    return;
  }

  /* Decode */
  int dec_size = strlen(data_str) * 3 / 4 + 1; /* decoded data length */
  uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded data */
  int rc = base64_decode(decoded, data_str, dec_size); /* decode */

  char *shellid_attr = (char *)hashmap_get(h, "s");
  if (shellid_attr == NULL) {
    werr("No shellid attribute in shells keys");
    return;
  }

  long int shellid = strtol(shellid_attr, &endptr, 10);
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", shellid_attr, shellid);
    return;
  }

  if (shells_vector[shellid] == NULL) {
    werr("Got keys from not existent shell");
    return;
  }

  /* Update xmpp context and connection in shell */
  pthread_mutex_lock(&shells_lock);
  shells_vector[shellid]->ctx = ctx;
  shells_vector[shellid]->conn = conn;
  pthread_mutex_unlock(&shells_lock);

  /* Send decoded data to screen */
  write(shells_vector[shellid]->fdm, decoded, rc);

  wlog("Return from shells_keys");
}

void send_shells_keys_response(char *data_str, int data_len, int shell_id) {
  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *keys = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(keys, "shells");
  xmpp_stanza_set_ns(keys, WNS);
  char shell_id_str[4];
  snprintf(shell_id_str, 4, "%d", shell_id);
  xmpp_stanza_set_attribute(keys, "shellid", shell_id_str);
  xmpp_stanza_set_attribute(keys, "action", "keys");

  xmpp_stanza_t *data = xmpp_stanza_new(ctx); /* data */
  char *encoded_data = (char *)malloc(BASE64_SIZE(data_len));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(data_len),
    (const unsigned char *)data_str, data_len);

  if (encoded_data == NULL) {
    werr("Could not encode");
    return;
  }

  xmpp_stanza_set_text(data, encoded_data);
  xmpp_stanza_add_child(keys, data);
  xmpp_stanza_add_child(message, keys);
  xmpp_send(conn, message);
  xmpp_stanza_release(keys);
  xmpp_stanza_release(data);
  xmpp_stanza_release(message);

  free(encoded_data);
}

void shells_poweroff() {
  if (strcmp(board_str, "server") != 0) {
    char cmd[64];
    snprintf(cmd, 64, "%s poweroff", sudo_str);
    system(cmd);
  }

  exit(EXIT_SUCCESS);
}

#endif /* SHELLS */
