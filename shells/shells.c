/**************************************************************************************************
 * Shells module implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifdef SHELLS


/*** INCLUDES ************************************************************************************/

#include <strings.h>
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
#include <sys/stat.h>
#include <sys/types.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* WNS */
#include "../base64/base64.h"         /* encode decode */
#include "../wtalk.h"                 /* RUNNING_PROJECTS_PATH */

#include "shells.h"                   /* shells module api */
#include "shells_helper.h"            /* read routine */
#include "wtalk_config.h"             /* version */

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *jid;
extern const char *owner;
extern const char *board;
extern const char *shell;
extern const char *run;
extern const char *build_file;

extern bool sudo;

extern xmpp_ctx_t *global_ctx;
extern xmpp_conn_t *global_conn;

extern bool is_xmpp_connection_set;

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static pthread_mutex_t shells_lock;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/**
 * Open shell
 */
static void shells_open(const char *from, xmpp_stanza_t *stanza);

/**
 * Build and sent shells open response
 */
static void send_shells_open_response(char *request_attr, bool success, int shell_id,
                                      bool running);

/**
 * Open normal shell
 */
static void open_normal_shell(char *request_attr, char *width_attr, char *height_attr);

/**
 * Open project shell
 */
static void open_project_shell(char *request_attr, char *width_attr, char *height_attr,
                               char *projectid_attr, char *userid_attr);

/**
 * Build array with words from str split by spaces. Append NULL at the end of the string.
 * Value of size will be the number of entries in the returned array.
 */
static char **string_to_array(char *str);

/**
 * Return concatenation of local environment variables and user environment variables.
 */
static char **concatenation_of_local_and_user_env(char **local_env, int local_env_size);

/**
 * Return index of first free shell entry from shells_vector.
 */
static int get_entry_in_shells_vector();

/**
 * Add new shell in shells_vector
 */
static bool allocate_memory_for_new_shell(int shell_index, int pid, int fdm,
                                          long int width, long int height,
                                          char *request_attr, char *projectid_attr,
                                          char *userid_attr);

/************************************************************************************************/



shell_t *shells_vector[MAX_SHELLS]; /* All shells */


extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);




void start_dead_projects(xmpp_conn_t *const conn, void *const userdata) {
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
      open_project_shell(NULL, NULL, NULL, projectid, NULL);
    }
    sleep(1);
  }

  fclose(fp);
}

void init_shells() {
  static bool shells_initialized = false;

  mkdir("/tmp/wyliodrin", 0700);

  if (!shells_initialized) {
    int i;

    pthread_mutex_lock(&shells_lock);
    for (i = 0; i < MAX_SHELLS; i++) {
      shells_vector[i] = NULL;
    }
    pthread_mutex_unlock(&shells_lock);

    shells_initialized = true;
  }
}

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
  werr2(strncasecmp(owner, from, strlen(owner)) != 0, return,
        "Ignore shells stanza received from %s", from);

  char *action_attr = xmpp_stanza_get_attribute(stanza, "action");
  werr2(action_attr == NULL, return, "Received shells stanza without action attribute");

  if (strncasecmp(action_attr, "open", 4) == 0) {
    shells_open(from, stanza);
  } else if (strncasecmp(action_attr, "close", 5) == 0) {
    shells_close(stanza, conn, userdata);
  } else if (strncasecmp(action_attr, "keys", 4) == 0) {
    shells_keys(stanza, conn, userdata);
  } else if (strncasecmp(action_attr, "list", 4) == 0) {
    shells_list(stanza, conn, userdata);
  } else if (strncasecmp(action_attr, "status", 6) == 0) {
    shells_status(stanza, conn, userdata);
  } else if (strncasecmp(action_attr, "poweroff", 8) == 0) {
    shells_poweroff();
  } else {
    werr("Received shells stanza with unknown action attribute %s from %s",
         action_attr, from);
  }
}

void shells_close(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_close(...)");

  char *endptr; /* strtol endptr */

  /* Get request attribute */
  char *request_attr = xmpp_stanza_get_attribute(stanza, "request"); /* request attribute */
  if(request_attr == NULL) {
    werr("Error while getting request attribute");
    return;
  }
  long int request = strtol(request_attr, &endptr, 10);
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", request_attr, request);
    return;
  }

  /* Get shellid attribute */
  char *shellid_attr = xmpp_stanza_get_attribute(stanza, "shellid"); /* shellid attribute */
  if(shellid_attr == NULL) {
    werr("Error while getting shellid attribute");
    return;
  }
  long int shellid = strtol(shellid_attr, &endptr, 10); /* shellid value */
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", shellid_attr, shellid);
    return;
  }

  /* Set close request or ignore it if it comes from unopened shell */
  pthread_mutex_lock(&shells_lock);
  if (shells_vector[shellid] != NULL) {
    shells_vector[shellid]->close_request = request;
    close(shells_vector[shellid]->fdm);
  } else {
    pthread_mutex_unlock(&shells_lock);
    return;
  }
  pthread_mutex_unlock(&shells_lock);

  /* Detach from screen session */
  if (xmpp_stanza_get_attribute(stanza, "background") == NULL) {
    int pid = fork();

    /* Return if fork failed */
    if (pid == -1) {
      werr("SYSERR fork");
      perror("fork");
      return;
    }

    /* Child from fork */
    if (pid == 0) {
      char screen_quit_cmd[32];
      if (strcmp(board, "raspberrypi") == 0) {
        snprintf(screen_quit_cmd, 31, "sudo kill -9 %d", shells_vector[shellid]->pid);
      } else {
        snprintf(screen_quit_cmd, 31, "kill -9 %d", shells_vector[shellid]->pid);
      }
      system(screen_quit_cmd);
      waitpid(shells_vector[shellid]->pid, NULL, 0);
      exit(EXIT_SUCCESS);
    }
    waitpid(pid, NULL, 0);
  }

  wlog("Return from shells_close");
}

void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_keys(...)");

  char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
  if (request_attr == NULL) {
    werr("Received keys with no request");
    return;
  }

  char *endptr; /* strtol endptr */

  char *data_str = xmpp_stanza_get_text(stanza); /* data string */
  if(data_str == NULL) {
    wlog("Return from shells_keys due to NULL data");
    return;
  }

  /* Decode */
  int dec_size = strlen(data_str) * 3 / 4 + 1; /* decoded data length */
  uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded data */
  int rc = base64_decode(decoded, data_str, dec_size); /* decode */

  char *shellid_attr = xmpp_stanza_get_attribute(stanza, "shellid");
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

  /* Update shell request */
  if (shells_vector[shellid]->request_attr != NULL) {
    free(shells_vector[shellid]->request_attr);
  }
  shells_vector[shellid]->request_attr = strdup(request_attr);

  /* Send decoded data to screen */
  write(shells_vector[shellid]->fdm, decoded, rc);

  wlog("Return from shells_keys");
}

void send_shells_keys_response(char *data_str, int data_len, int shell_id) {
  while (!is_xmpp_connection_set) {
    usleep(500000);
  }

  if (shells_vector[shell_id]->request_attr == NULL) {
    werr("Trying to send keys but shell has no request attribute");
    return;
  }

  xmpp_stanza_t *message = xmpp_stanza_new(global_ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner);
  xmpp_stanza_t *keys = xmpp_stanza_new(global_ctx); /* shells action done stanza */
  xmpp_stanza_set_name(keys, "shells");
  xmpp_stanza_set_ns(keys, WNS);
  char shell_id_str[8];
  snprintf(shell_id_str, 7, "%d", shell_id);
  xmpp_stanza_set_attribute(keys, "shellid", shell_id_str);
  xmpp_stanza_set_attribute(keys, "action", "keys");
  xmpp_stanza_set_attribute(keys, "request", shells_vector[shell_id]->request_attr);
  xmpp_stanza_t *data = xmpp_stanza_new(global_ctx); /* data */
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
  xmpp_send(global_conn, message);
  xmpp_stanza_release(keys);
  xmpp_stanza_release(data);
  xmpp_stanza_release(message);

  free(encoded_data);
}

void shells_list(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_list(...)");

  /* Send list */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with close */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner);
  xmpp_stanza_t *list = xmpp_stanza_new(ctx); /* close stanza */
  xmpp_stanza_set_name(list, "shells");
  xmpp_stanza_set_ns(list, WNS);
  xmpp_stanza_set_attribute(list, "action", "list");
  xmpp_stanza_set_attribute(list, "request",
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_t *project = xmpp_stanza_new(ctx); /* project stanza */
  xmpp_stanza_set_attribute(project, "projectid", "0");
  xmpp_stanza_add_child(list, project);
  xmpp_stanza_add_child(message, list);
  xmpp_send(conn, message);
  xmpp_stanza_release(list);
  xmpp_stanza_release(message);

  wlog("Return from shell_list");
}

void shells_status(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid");

  if (projectid_attr != NULL) {
    char projectid_filepath[128];
    snprintf(projectid_filepath, 127, "/tmp/wyliodrin/%s", projectid_attr);
    int projectid_fd = open(projectid_filepath, O_RDWR);
    bool is_project_running = projectid_fd != -1;
    if (is_project_running) {
      close(projectid_fd);
    }

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

    xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx); /* message with close */
    xmpp_stanza_set_name(message_stz, "message");
    xmpp_stanza_set_attribute(message_stz, "to", owner);

    xmpp_stanza_t *status_stz = xmpp_stanza_new(ctx); /* status stanza */
    xmpp_stanza_set_name(status_stz, "shells");
    xmpp_stanza_set_ns(status_stz, WNS);
    xmpp_stanza_set_attribute(status_stz, "action", "status");
    xmpp_stanza_set_attribute(status_stz, "request",
      (const char *)xmpp_stanza_get_attribute(stanza, "request"));
    xmpp_stanza_set_attribute(status_stz, "projectid",
      (const char *)xmpp_stanza_get_attribute(stanza, "projectid"));
    xmpp_stanza_set_attribute(status_stz, "running",
      is_project_running ? "true" : "false");

    xmpp_stanza_add_child(message_stz, status_stz);
    xmpp_send(conn, message_stz);
    xmpp_stanza_release(status_stz);
    xmpp_stanza_release(message_stz);
  } else {
    werr("No projectid attribute in status");
  }
}

void shells_poweroff() {
  if (strcmp(board, "server") != 0) {
    char cmd[64];
    snprintf(cmd, 63, "%spoweroff", strcmp(board, "raspberrypi") == 0 ? "sudo " : "");
    system(cmd);
  }

  exit(EXIT_SUCCESS);
}


/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/


static void shells_open(const char *from, xmpp_stanza_t *stanza) {
  char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
  werr2(request_attr == NULL, goto _error,
        "Received shells open stanza from %s without request attribute", from);

  char *width_attr = xmpp_stanza_get_attribute(stanza, "width");
  werr2(width_attr == NULL, goto _error,
        "Received shells open stanza from %s without width attribute", from);

  char *height_attr = xmpp_stanza_get_attribute(stanza, "height");
  werr2(height_attr == NULL, goto _error,
        "Received shells open stanza from %s without height attribute", from);

  char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid");
  char *userid_attr    = xmpp_stanza_get_attribute(stanza, "userid");

  if (projectid_attr == NULL) {
    open_normal_shell(request_attr, width_attr, height_attr);
  } else {
    open_project_shell(request_attr, width_attr, height_attr,
      projectid_attr, userid_attr);
  }

  _error: ;
    send_shells_open_response(request_attr, false, 0, false);
}


static void send_shells_open_response(char *request_attr, bool success, int shell_id, bool running) {
  xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", owner);
  xmpp_stanza_t *shells_stz = xmpp_stanza_new(global_ctx);
  xmpp_stanza_set_name(shells_stz, "shells");
  xmpp_stanza_set_ns(shells_stz, WNS);
  xmpp_stanza_set_attribute(shells_stz, "action", "open");
  xmpp_stanza_set_attribute(shells_stz, "request", request_attr);
  if (success) {
    xmpp_stanza_set_attribute(shells_stz, "response", "done");
    char shell_id_str[8];
    snprintf(shell_id_str, 8, "%d", shell_id);
    xmpp_stanza_set_attribute(shells_stz, "shellid", shell_id_str);
    xmpp_stanza_set_attribute(shells_stz, "running", running ? "true" : "false");
  } else {
    xmpp_stanza_set_attribute(shells_stz, "response", "error");
  }
  xmpp_stanza_add_child(message_stz, shells_stz);
  xmpp_send(global_conn, message_stz);
  xmpp_stanza_release(shells_stz);
  xmpp_stanza_release(message_stz);
}


static void open_normal_shell(char *request_attr, char *width_attr, char *height_attr) {
  /* Get width and height for winsize */
  char *endptr;

  long width = strtol(width_attr, &endptr, 10);
  werr2(*endptr != '\0', goto _error, "Invalid width attribute: %s", width_attr);

  long height = strtol(height_attr, &endptr, 10);
  werr2(*endptr != '\0', goto _error, "Invalid height attribute: %s", height_attr);

  struct winsize ws = { height, width, 0, 0 };

  /* Fork */
  int fdm;
  int pid = forkpty(&fdm, NULL, NULL, &ws);
  wsyserr2(pid == -1, goto _error, "Forkpty failed");

  if (pid == 0) { /* Child from forkpty */
    /* Build local environment variables */
    char wyliodrin_board_env[64];
    snprintf(wyliodrin_board_env, 64, "wyliodrin_board=%s", board);

    char wyliodrin_server[64];
    snprintf(wyliodrin_server, 64, "wyliodrin_server=%d.%d",
             WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR);

    char *local_env[] = { wyliodrin_board_env, wyliodrin_server, "HOME=/wyliodrin", "TERM=xterm",
                          NULL };

    /* Build all environment variables */
    int local_env_size = sizeof(local_env) / sizeof(*local_env);
    char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

    chdir("/wyliodrin");
    char **exec_argv = string_to_array((char *)shell);
    execvpe(exec_argv[0], exec_argv, all_env);

    wsyserr2(true, /* Do nothing */, "Running the shell command failed");
    exit(EXIT_FAILURE);
  }

  /* NOTE: If an error occurs after this point, the child process should be killed. */

  /* Get an entry in the shells_vector */
  pthread_mutex_lock(&shells_lock);

  int shell_index = get_entry_in_shells_vector();
  werr2(shell_index == MAX_SHELLS, goto _error, "Only %d open shells are allowed", MAX_SHELLS);

  bool new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
                                                          request_attr, NULL, NULL);
  werr2(!new_shell_alloc_rc, goto _error, "Could not add new shell");

  pthread_mutex_unlock(&shells_lock);

  /* Create new thread for read routine */
  pthread_t rt;
  int rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[shell_index]);
  wsyserr2(rc < 0, goto _error, "Could not create thread for read routine");
  pthread_detach(rt);

  winfo("Open new shell");
  send_shells_open_response(request_attr, true, shell_index, false);

  return;

  _error:
    werr("Failed to open new shell");
    send_shells_open_response(request_attr, false, 0, false);
}


static void open_project_shell(char *request_attr, char *width_attr, char *height_attr,
                               char *projectid_attr, char *userid_attr) {
  /* Sanity checks */
  werr2(request_attr   == NULL, return, "Trying to open new project with NULL request");
  werr2(width_attr     == NULL, return, "Trying to open new project with NULL width");
  werr2(height_attr    == NULL, return, "Trying to open new project with NULL height");
  werr2(projectid_attr == NULL, return, "Trying to open new project with NULL projectid");
  werr2(userid_attr    == NULL, return, "Trying to open new project with NULL userid");

  winfo("Starting project %s", projectid_attr);

  /* Get the shell_index in case of a running project */
  int shell_index;
  char projectid_filepath[128];
  snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);
  if (request_attr != NULL) {
    int projectid_fd = open(projectid_filepath, O_RDONLY);
    if (projectid_fd != -1) {
      read(projectid_fd, &shell_index, sizeof(int));
      close(projectid_fd);

      winfo("Project %s is running", projectid_attr);
      send_shells_open_response(request_attr, true, shell_index, true);
      return;
    }
  }

  /* Get width and height for winsize */
  char *endptr;

  long width = strtol(width_attr, &endptr, 10);
  werr2(*endptr != '\0', goto _error, "Invalid width attribute: %s", width_attr);

  long height = strtol(height_attr, &endptr, 10);
  werr2(*endptr != '\0', goto _error, "Invalid height attribute: %s", height_attr);

  struct winsize ws = { height, width, 0, 0 };

  /* Fork */
  int fdm;
  int pid = forkpty(&fdm, NULL, NULL, &ws);
  wsyserr2(pid == -1, goto _error, "Forkpty failed");

  if (pid == 0) { /* Child from forkpty */
    /* Write the shells index in the tmp project's file */
    int projectid_fd = open(projectid_filepath, O_CREAT | O_WRONLY, 0600);
    wsyserr2(projectid_fd == -1, sleep(3); exit(EXIT_FAILURE),
             "Could not open %s", projectid_filepath);
    write(projectid_fd, &shell_index, sizeof(int));
    close(projectid_fd);

    /* Build local environment variables */
    char wyliodrin_project_env[64];
    snprintf(wyliodrin_project_env, 64, "wyliodrin_project=%s", projectid_attr);
    char wyliodrin_userid_env[64];
    snprintf(wyliodrin_userid_env, 64, "wyliodrin_userid=%s", userid_attr);
    char wyliodrin_session_env[64];
    snprintf(wyliodrin_session_env, 64, "wyliodrin_session=%s", request_attr);
    char wyliodrin_board_env[64];
    snprintf(wyliodrin_board_env, 64, "wyliodrin_board=%s", board);
    char wyliodrin_jid_env[64];
    snprintf(wyliodrin_jid_env, 64, "wyliodrin_jid=%s", jid);
    char wyliodrin_server[64];
    snprintf(wyliodrin_server, 64, "wyliodrin_server=%d.%d",
             WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR);

    #ifdef USEMSGPACK
      char wyliodrin_usemsgpack_env[64];
      snprintf(wyliodrin_usemsgpack_env, 64, "wyliodrin_usemsgpack=1");

      char *local_env[] = { wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
        wyliodrin_board_env, wyliodrin_jid_env, wyliodrin_server, "HOME=/wyliodrin", "TERM=xterm",
        wyliodrin_usemsgpack_env, NULL };
    #else
      char *local_env[] = { wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
        wyliodrin_board_env, wyliodrin_jid_env, wyliodrin_server, "HOME=/wyliodrin", "TERM=xterm",
        NULL };
    #endif

    int local_env_size = sizeof(local_env) / sizeof(*local_env);
    char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

    char **exec_argv = string_to_array((char *)run);

    char cd_path[256];
    snprintf(cd_path, 256, "%s/%s", build_file, projectid_attr);
    chdir(cd_path);

    execvpe(exec_argv[0], exec_argv, all_env);

    wsyserr2(true, /* Do nothing */, "Running the run command failed");
    exit(EXIT_FAILURE);
  }

  /* NOTE: If an error occurs after this point, the child process should be killed. */

  /* Get an entry in the shells_vector */
  pthread_mutex_lock(&shells_lock);

  shell_index = get_entry_in_shells_vector();
  werr2(shell_index == MAX_SHELLS, goto _error, "Only %d open shells are allowed", MAX_SHELLS);

  bool new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
                                                          request_attr, projectid_attr,
                                                          userid_attr);
  werr2(!new_shell_alloc_rc, goto _error, "Could not add new shell");

  pthread_mutex_unlock(&shells_lock);

  /* Write the project in RUNNING_PROJECTS_PATH */
  int open_rc = open(RUNNING_PROJECTS_PATH, O_WRONLY | O_APPEND);
  wsyserr2(open_rc == -1, goto _error, "Failed to open %s", RUNNING_PROJECTS_PATH);
  char projectid_attr_with_colon[64];
  sprintf(projectid_attr_with_colon, "%s:", projectid_attr);
  write(open_rc, projectid_attr_with_colon, strlen(projectid_attr_with_colon));

  /* Create new thread for read routine */
  pthread_t rt;
  int rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[shell_index]);
  wsyserr2(rc > 0, goto _error, "Failed to create new thread");
  pthread_detach(rt);

  /* Send success response */
  send_shells_open_response(request_attr, true, shell_index, false);

  winfo("Open project %s", projectid_attr);
  return;

  _error: ;
    winfo("Failed to open project %s", projectid_attr);
    send_shells_open_response(request_attr, false, -1, false);
}


static char **string_to_array(char *str) {
  char **return_value = NULL;
  int num_spaces = 0;

  char *saveptr;
  char *p = strtok_r(str, " ", &saveptr);
  while (p) {
    num_spaces++;
    return_value = realloc(return_value, sizeof(char*) * num_spaces);
    wsyserr2(return_value == NULL, return NULL, "Could not reallocate memory");

    return_value[num_spaces-1] = p;

    p = strtok_r(NULL, " ", &saveptr);
  }

  num_spaces++;
  return_value = realloc(return_value, sizeof(char*) * num_spaces);
  wsyserr2(return_value == NULL, return NULL, "Could not reallocate memory");
  return_value[num_spaces-1] = NULL;

  return return_value;
}


static char **concatenation_of_local_and_user_env(char **local_env, int local_env_size) {
  int environ_size = 0;
  while (environ[environ_size]) {
    environ_size++;
  }

  char **all_env = malloc((environ_size + local_env_size) * sizeof(char *));
  wsyserr2(all_env == NULL, return NULL, "Could not allocate memory for environment variables");
  memcpy(all_env, environ, environ_size * sizeof(char *));
  memcpy(all_env + environ_size, local_env, local_env_size * sizeof(char *));

  return all_env;
}


static int get_entry_in_shells_vector() {
  int shell_index;

  for (shell_index = 0; shell_index < MAX_SHELLS; shell_index++) {
    if (shells_vector[shell_index] == NULL) {
      break;
    }
  }

  return shell_index;
}


static bool allocate_memory_for_new_shell(int shell_index, int pid, int fdm,
                                          long int width, long int height,
                                          char *request_attr, char *projectid_attr,
                                          char *userid_attr) {
  werr2(shells_vector[shell_index] != NULL, return false,
        "Trying to allocate memory for new shell with index %d, but shell is still in use",
        shell_index);

  shells_vector[shell_index] = malloc(sizeof(shell_t));
  wsyserr2(shells_vector[shell_index] == NULL, return false,
           "Allocation of memory for new shell failed");

  shells_vector[shell_index]->id             = shell_index;
  shells_vector[shell_index]->pid            = pid;
  shells_vector[shell_index]->fdm            = fdm;
  shells_vector[shell_index]->width          = width;
  shells_vector[shell_index]->height         = height;
  shells_vector[shell_index]->request_attr   = strdup(request_attr);
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
  shells_vector[shell_index]->close_request  = -1;

  return true;
}

/*************************************************************************************************/



#endif /* SHELLS */
