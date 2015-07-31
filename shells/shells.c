/**************************************************************************************************
 * Shells module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
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
#include "shells.h"                   /* shells module api */
#include "shells_helper.h"            /* read routine */



/* Variables from wtalk.c */
extern const char *build_file_str; /* build_file_str */
extern const char *board_str;      /* board name */
extern const char *jid_str;        /* jid */
extern const char *owner_str;      /* owner_str */
extern const char *sudo_str;       /* sudo command from wtalk.c */

shell_t *shells_vector[MAX_SHELLS]; /* All shells */

pthread_mutex_t shells_lock; /* shells mutex */

extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);

void init_shells()
{
  static bool shells_initialized = false;

  mkdir("/tmp/wyliodrin", 0700);

  if (!shells_initialized) {
    uint32_t i;

    pthread_mutex_lock(&shells_lock);
    for(i = 0; i < MAX_SHELLS; i++) {
      shells_vector[i] = NULL;
    }
    pthread_mutex_unlock(&shells_lock);

    shells_initialized = true;
  }
}

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells(%s, %s, %d, stanza)", from, to, error);

  char *action_attr = xmpp_stanza_get_attribute(stanza, "action"); /* action attribute */
  if (action_attr == NULL) {
    werr("Error while getting action attribute");
    return;
  }

  if(strncasecmp(action_attr, "open", 4) == 0) {
    shells_open(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "close", 5) == 0) {
    shells_close(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "keys", 4) == 0) {
    shells_keys(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "list", 4) == 0) {
    shells_list(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "status", 6) == 0) {
    shells_status(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "poweroff", 8) == 0) {
    shells_poweroff();
  } else {
    werr("Unknown action attribute: %s", action_attr);
  }

  wlog("Return from shells");
}

void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_open(...)");

  char *endptr; /* strtol endptr */

  /* Get request attribute */
  char *request_attr = xmpp_stanza_get_attribute(stanza, "request"); /* request attribute */
  if(request_attr == NULL) {
    werr("Error while getting request attribute");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }
  long int request = strtol(request_attr, &endptr, 10); /* request value */
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", request_attr, request);
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }

  /* Get width attribute */
  char *w_attr = xmpp_stanza_get_attribute(stanza, "width"); /* width attribute */
  if (w_attr == NULL) {
    werr("Error while getting width attribute");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }
  long int w = strtol(w_attr, &endptr, 10); /* width value */
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", w_attr, w);
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }
  if (w == 0) {
    werr("width is 0");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }

  /* Get height attribute */
  char *h_attr = xmpp_stanza_get_attribute(stanza, "height"); /* height attribute */
  if (h_attr == NULL) {
    werr("Error while getting height attribute");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }
  long int h = strtol(h_attr, &endptr, 10); /* height value */
  if (*endptr != '\0') {
    werr("strtol error: str = %s, val = %ld", h_attr, h);
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }
  if (h == 0) {
    werr("height is 0");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
    return;
  }

  uint32_t shell_index; /* shell_t index in shells_vector */

  bool projectid_running = false;
  char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid"); /* projectid attribute */
  char projectid_filepath[64];
  if (projectid_attr != NULL) {
    /* A make shell must be opened */
    sprintf(projectid_filepath, "/tmp/wyliodrin/%s", projectid_attr);
    int projectid_fd = open(projectid_filepath, O_RDWR);
    if (projectid_fd != -1) {
      read(projectid_fd, &shell_index, sizeof(uint32_t));
      wlog("shell_index = %u", shell_index);
      projectid_running = true;
    }
  }

  /* Get an entry in shells_vector */
  if (projectid_running == false) {
    pthread_mutex_lock(&shells_lock);
    for (shell_index = 0; shell_index < MAX_SHELLS; shell_index++) {
      if (shells_vector[shell_index] == NULL) {
        break;
      }
    }
    pthread_mutex_unlock(&shells_lock);

    if (shell_index == MAX_SHELLS) {
      werr("No shells left");
      send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
      return;
    }
  }

  /* Open screen in new pseudoterminal */
  int fdm;                                  /* Master part of PTY */
  struct winsize ws = {h, w, 0, 0};         /* Window size */
  int pid;

  if ((projectid_attr == NULL) ||
      (projectid_attr != NULL && projectid_running == false)) {
    pid = forkpty(&fdm, NULL, NULL, &ws); /* pid of parent from forkpty */

    if (pid == -1) { /* Error in forkpty */
      werr("SYSERR forkpty");
      perror("forkpty");
      send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
      return;
    }

    if (pid == 0) { /* Child */
       /* Child */
      /* A make shell must be opened */
      if (projectid_attr != NULL) {

        int projectid_fd = open(projectid_filepath, O_CREAT | O_RDWR, 0600);
        wsyserr(projectid_fd == -1, "open projectid_filepath");
        write(projectid_fd, &shell_index, sizeof(uint32_t));

        char *userid_attr = xmpp_stanza_get_attribute(stanza, "userid");
        char cd_path[256];
        sprintf(cd_path, "%s/%s", build_file_str, projectid_attr);
        int rc = chdir(cd_path);
        wsyserr(rc == -1, "chdir");

        char makefile_name[50];
        sprintf(makefile_name, "Makefile.%s", board_str);

        char *make_run[] = {"make", "-f", makefile_name, "run", NULL};

        char wyliodrin_project_env[64];
        sprintf(wyliodrin_project_env,"wyliodrin_project=%s", projectid_attr);

        char wyliodrin_userid_env[64];
        sprintf(wyliodrin_userid_env,"wyliodrin_userid=%s", userid_attr);

        char wyliodrin_session_env[64];
        sprintf(wyliodrin_session_env,"wyliodrin_session=%s", request_attr);

        char wyliodrin_board_env[64];
        sprintf(wyliodrin_board_env, "wyliodrin_board=%s", board_str);

        char wyliodrin_jid_env[64];
        sprintf(wyliodrin_jid_env, "wyliodrin_jid=%s", jid_str);

        char home_env[] = "HOME=/wyliodrin";
        char term_env[] = "TERM=xterm";

        #ifdef USEMSGPACK
          char wyliodrin_usemsgpack_env[64];
          sprintf(wyliodrin_usemsgpack_env, "wyliodrin_usemsgpack=1");
          char *env[] = { wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
            wyliodrin_board_env, wyliodrin_jid_env, home_env, term_env, wyliodrin_usemsgpack_env,
            NULL};
        #else
          char *env[] = {wyliodrin_project_env, wyliodrin_userid_env, wyliodrin_session_env,
            wyliodrin_board_env, wyliodrin_jid_env, home_env, term_env, NULL};
        #endif

        int env_size = sizeof(env) / sizeof(*env);

        int environ_size = 0;
        while(environ[environ_size]) {
          environ_size++;
        }

        char **all_env = malloc((environ_size + env_size) * sizeof(char *));
        memcpy(all_env, environ, environ_size * sizeof(char *));
        memcpy(all_env + environ_size, env, env_size * sizeof(char *));

        execvpe(make_run[0], make_run, all_env);

        exit(EXIT_FAILURE);
      }

      /* A normal shell must be opened */
      else {
        char shell_name[256];
        sprintf(shell_name, "shell%d", shell_index);
        char *args[] = {"bash", NULL};

        char wyliodrin_board_env [50];
        sprintf(wyliodrin_board_env, "wyliodrin_board=%s",board_str);

        char home_env[] = "HOME=/wyliodrin";
        char term_env[] = "TERM=xterm";

        char *env[] = {wyliodrin_board_env, home_env, term_env, NULL};

        int env_size = sizeof(env) / sizeof(*env);

        int environ_size = 0;
        while(environ[environ_size]) {
          environ_size++;
        }

        char **all_env = malloc((environ_size + env_size) * sizeof(char *));
        memcpy(all_env, environ, environ_size * sizeof(char *));
        memcpy(all_env + environ_size, env, env_size * sizeof(char *));

        execvpe(args[0], args, all_env);

        werr("bash failed");
        exit(EXIT_FAILURE);
      }
      return;
    }
  }

  /* Parent in forkpty */
  if (projectid_running == false) {
    pthread_mutex_lock(&shells_lock);
    shells_vector[shell_index]                = (shell_t *)malloc(sizeof(shell_t));
    shells_vector[shell_index]->pid           = pid;
    shells_vector[shell_index]->id            = shell_index;
    shells_vector[shell_index]->request_id    = request;
    shells_vector[shell_index]->fdm           = fdm;
    shells_vector[shell_index]->conn          = conn;
    shells_vector[shell_index]->ctx           = (xmpp_ctx_t *)userdata;
    shells_vector[shell_index]->close_request = -1;
    if (projectid_attr != NULL) {
      shells_vector[shell_index]->projectid   = strdup(projectid_attr);
    } else {
      shells_vector[shell_index]->projectid   = NULL;
    }
    pthread_mutex_unlock(&shells_lock);

    /* Create new thread for read routine */
    pthread_t rt; /* Read thread */
    int rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[shell_index]); /* Read rc */
    if (rc < 0) {
      werr("SYSERR pthread_create");
      perror("pthread_create");
      send_shells_open_response(stanza, conn, userdata, FALSE, -1, false);
      return;
    }
    pthread_detach(rt);
  }

  send_shells_open_response(stanza, conn, userdata, TRUE, shell_index, projectid_running);

  wlog("Return success from shells_open");
}

void send_shells_open_response(xmpp_stanza_t *stanza, xmpp_conn_t *const conn,
    void *const userdata, int8_t success, int8_t id, bool running) {

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *done = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(done, "shells");
  xmpp_stanza_set_ns(done, WNS);
  xmpp_stanza_set_attribute(done, "action", "open");
  if (success == TRUE) {
    xmpp_stanza_set_attribute(done, "response", "done");
    char id_str[4];
    sprintf(id_str, "%d", id);
    xmpp_stanza_set_attribute(done, "shellid", id_str);
  } else {
    xmpp_stanza_set_attribute(done, "response", "error");
  }
  xmpp_stanza_set_attribute(done, "request",
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_set_attribute(done, "running", running ? "true" : "false");
  xmpp_stanza_add_child(message, done);
  xmpp_send(conn, message);
  xmpp_stanza_release(done);
  xmpp_stanza_release(message);
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
      char screen_quit_cmd[256];
      // sprintf(screen_quit_cmd, "screen -S shell%ld -X quit", shellid);
      sprintf(screen_quit_cmd, "kill -9 %d", shells_vector[shellid]->pid);
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

  /* Update xmpp context and connection in shell */
  pthread_mutex_lock(&shells_lock);
  shells_vector[shellid]->ctx = (xmpp_ctx_t *)userdata;
  shells_vector[shellid]->conn = conn;
  pthread_mutex_unlock(&shells_lock);

  /* Send decoded data to screen */
  write(shells_vector[shellid]->fdm, decoded, rc);

  wlog("Return from shells_keys");
}

void send_shells_keys_response(xmpp_conn_t *const conn, void *const userdata,
    char *data_str, int data_len, int shell_id) {
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
  if (ctx == NULL) {
    wlog("NULL xmpp context");
    return;
  }

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *keys = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(keys, "shells");
  xmpp_stanza_set_ns(keys, WNS);
  char shell_id_str[4];
  sprintf(shell_id_str, "%d", shell_id);
  xmpp_stanza_set_attribute(keys, "shellid", shell_id_str);
  xmpp_stanza_set_attribute(keys, "action", "keys");

  xmpp_stanza_t *data = xmpp_stanza_new(ctx); /* data */
  char *encoded_data = (char *)malloc(BASE64_SIZE(data_len));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(data_len),
    (const unsigned char *)data_str, data_len);

  if (encoded_data == NULL) {
    werr("Could not encode\n");
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

void shells_list(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_list(...)");

  /* Send list */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with close */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
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
    char projectid_filepath[64];
    sprintf(projectid_filepath, "/tmp/wyliodrin/%s", projectid_attr);

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

    xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx); /* message with close */
    xmpp_stanza_set_name(message_stz, "message");
    xmpp_stanza_set_attribute(message_stz, "to", owner_str);

    xmpp_stanza_t *status_stz = xmpp_stanza_new(ctx); /* status stanza */
    xmpp_stanza_set_name(status_stz, "shells");
    xmpp_stanza_set_ns(status_stz, WNS);
    xmpp_stanza_set_attribute(status_stz, "action", "status");
    xmpp_stanza_set_attribute(status_stz, "request",
      (const char *)xmpp_stanza_get_attribute(stanza, "request"));
    xmpp_stanza_set_attribute(status_stz, "projectid",
      (const char *)xmpp_stanza_get_attribute(stanza, "projectid"));
    xmpp_stanza_set_attribute(status_stz, "running",
      open(projectid_filepath, O_RDWR) != -1 ? "true" : "false");

    xmpp_stanza_add_child(message_stz, status_stz);
    xmpp_send(conn, message_stz);
    xmpp_stanza_release(status_stz);
    xmpp_stanza_release(message_stz);
  } else {
    werr("No projectid attribute in status");
  }
}

void shells_poweroff() {
  if (strcmp(board_str, "server") != 0) {
    char cmd[64];
    sprintf(cmd, "%s poweroff", sudo_str);
    system(cmd);
  }

  exit(EXIT_SUCCESS);
}

#endif /* SHELLS */
