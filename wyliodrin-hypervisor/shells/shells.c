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
#include "../cmp/cmp.h"               /* msgpack handling */
// #include "../wxmpp/wxmpp.h"           /* WNS */
// #include "../base64/base64.h"         /* encode decode */
// #include "../wtalk.h"                 /* RUNNING_PROJECTS_PATH */

#include "shells.h"                   /* shells module api */
// #include "wtalk_config.h"             /* version */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define BUFSIZE        1024   /* Size of buffer used in the read routine */

#define DEFAULT_WIDTH  "12"   /* Default shells width  */
#define DEFAULT_HEIGHT "103"  /* Default shells height */

/*************************************************************************************************/



/*** TYPEDEFS ************************************************************************************/

typedef enum {
  SHELL,
  PROJECT
} shell_type_t;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

// extern const char *jid;
// extern const char *home;
// extern const char *owner;
// extern const char *board;
// extern const char *shell;
// extern const char *run;
// extern const char *stop;
// extern const char *build_file;

// extern xmpp_ctx_t *global_ctx;
// extern xmpp_conn_t *global_conn;

// extern bool is_xmpp_connection_set;

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static pthread_mutex_t shells_lock;

/*************************************************************************************************/


/*** VARIABLES ***********************************************************************************/

shell_t *shells_vector[MAX_SHELLS];

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/**
 * Open shell
 */
static void shells_open(const char *data);

// /**
//  * Build and sent shells open response
//  */
// static void send_shells_open_response(char *request_attr, bool success, int shell_id,
//                                       bool running);

// /**
//  * Open shell or project
//  */
// static void open_shell_or_project(shell_type_t shell_type, char *request_attr,
//                                   char *width_attr, char *height_attr,
//                                   char *projectid_attr, char *userid_attr);

/**
 * Close shell
 */
static void shells_close(const char *data);

// /**
//  * Send shells close response
//  */
// static void send_shells_close_response(char *request_attr, char *shellid_attr, char *code);

/**
 * Get keys.
 */
static void shells_keys(const char *data);

/**
 * Status of project (running or not).
 */
static void shells_status(const char *data);

/**
 * Disconnect shell.
 */
static void shells_disconnect(const char *data);

/**
 * Poweroff board.
 */
static void shells_poweroff();

// /**
//  * Send shells keys response.
//  */
// static void send_shells_keys_response(char *data_str, int data_len, int shell_id);

// /**
//  * Build array with words from str split by spaces. Append NULL at the end of the string.
//  * Value of size will be the number of entries in the returned array.
//  */
// static char **string_to_array(char *str);

// /**
//  * Return concatenation of local environment variables and user environment variables.
//  */
// static char **concatenation_of_local_and_user_env(char **local_env, int local_env_size);

// /**
//  * Return index of first free shell entry from shells_vector.
//  */
// static int get_entry_in_shells_vector();

// /**
//  * Add new shell in shells_vector
//  */
// static bool allocate_memory_for_new_shell(int shell_index, int pid, int fdm,
//                                           long int width, long int height,
//                                           char *request_attr, char *projectid_attr,
//                                           char *userid_attr);

// /**
//  * Generic build local env
//  */
// static char **build_local_env(shell_type_t shell_type, int *num_env,
//                               char *request_attr, char *projectid_attr, char *userid_attr);

// /**
//  * Build local env for shell.
//  */
// static char **build_local_env_for_shell(int *num_env, char *request_attr, char *projectid_attr,
//                                         char *userid_attr);

// /**
//  * Build local env for project.
//  */
// static char **build_local_env_for_project(int *num_env, char *request_attr, char *projectid_attr,
//                                           char *userid_attr);

// /**
//  * Remove project id from running projects file.
//  */
// static void remove_project_id_from_running_projects(char *projectid_attr);

// /**
//  * Routine for shell or project.
//  */
// static void *read_routine(void *args);

/************************************************************************************************/



/*** ENVIRONMENT *********************************************************************************/

extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);

/************************************************************************************************/


/*** API IMPLEMENTATION **************************************************************************/

void init_shells() {
  mkdir("/tmp/wyliodrin", 0700);

  int i;
  for (i = 0; i < MAX_SHELLS; i++) {
    shells_vector[i] = NULL;
  }
}


void shells(const char *data) {
  // werr2(strncasecmp(owner, from, strlen(owner)) != 0, return,
  //       "Ignore shells stanza received from %s", from);

  // char *action_attr = xmpp_stanza_get_attribute(stanza, "action");
  // werr2(action_attr == NULL, return, "Received shells stanza without action attribute");

  // if (strncasecmp(action_attr, "open", 4) == 0) {
  //   shells_open(from, stanza);
  // } else if (strncasecmp(action_attr, "close", 5) == 0) {
  //   shells_close(from, stanza);
  // } else if (strncasecmp(action_attr, "keys", 4) == 0) {
  //   shells_keys(from, stanza);
  // } else if (strncasecmp(action_attr, "status", 6) == 0) {
  //   shells_status(from, stanza);
  // } else if (strncasecmp(action_attr, "poweroff", 8) == 0) {
  //   shells_poweroff();
  // } else if (strncasecmp(action_attr, "disconnect", 10) == 0) {
  //   shells_disconnect(from, stanza);
  // } else {
  //   werr("Received shells stanza with unknown action attribute %s from %s",
  //        action_attr, from);
  // }

  cmp_ctx_t cmp;
  cmp_init(&cmp, (void *)data, strlen(data));

  uint32_t array_size;
  werr2(!cmp_read_array(&cmp, &array_size),
        return,
        "cmp_read_array error: %s", cmp_strerror(&cmp));

  werr2(array_size < 2, return, "Received array with less than 2 values");

  char *str = NULL;

  /* Read stanza name */
  werr2(!cmp_read_str(&cmp, &str),
        return,
        "cmp_read_str error: %s", cmp_strerror(&cmp));
  free(str);

  /* Read stanza text */
  werr2(!cmp_read_str(&cmp, &str),
        return,
        "cmp_read_str error: %s", cmp_strerror(&cmp));
  free(str);

  /* Read attributes */
  int i;
  bool action_found = false;
  for (i = 0; i < (array_size - 2) / 2; i++) {
    /* Read key */
    werr2(!cmp_read_str(&cmp, &str),
          return,
          "cmp_read_str error: %s", cmp_strerror(&cmp));
    if (strncmp(str, "action", 6) == 0) {
      /* Read action value */
      free(str);
      werr2(!cmp_read_str(&cmp, &str),
            return,
            "cmp_read_str error: %s", cmp_strerror(&cmp));

      if (strncasecmp(str, "open", 4) == 0) {
        shells_open(data);
      } else if (strncasecmp(str, "close", 5) == 0) {
        shells_close(data);
      } else if (strncasecmp(str, "keys", 4) == 0) {
        shells_keys(data);
      } else if (strncasecmp(str, "status", 6) == 0) {
        shells_status(data);
      } else if (strncasecmp(str, "poweroff", 8) == 0) {
        shells_poweroff();
      } else if (strncasecmp(str, "disconnect", 10) == 0) {
        shells_disconnect(data);
      } else {
        werr("Received shells stanza with unknown action attribute %s", str);
      }

      free(str);
      action_found = true;
      break;
    } else {
      /* Read value */
      free(str);
      werr2(!cmp_read_str(&cmp, &str),
            return,
            "cmp_read_str error: %s", cmp_strerror(&cmp));
    }
  }
  if (!action_found) {
    werr("There is no action attribute in shells stanza");
  }
}


void start_dead_projects() {
  // FILE *fp = fopen(RUNNING_PROJECTS_PATH, "r");
  // werr2(fp == NULL, return, "Could no open %s", RUNNING_PROJECTS_PATH);

  // char projectid[128];
  // while (fscanf(fp, "%[^:]:", projectid) != EOF) {
  //   if (strlen(projectid) > 0) {
  //     remove_project_id_from_running_projects(projectid);
  //     winfo("Starting project %s", projectid);
  //     open_shell_or_project(PROJECT, NULL, DEFAULT_WIDTH, DEFAULT_HEIGHT, projectid, NULL);
  //   }
  // }

  // fclose(fp);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void shells_open(const char *data) {
  printf("shells_open\n");
//   char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
//   werr2(request_attr == NULL, goto _error,
//         "Received shells open stanza from %s without request attribute", from);

//   char *width_attr = xmpp_stanza_get_attribute(stanza, "width");
//   werr2(width_attr == NULL, goto _error,
//         "Received shells open stanza from %s without width attribute", from);

//   char *height_attr = xmpp_stanza_get_attribute(stanza, "height");
//   werr2(height_attr == NULL, goto _error,
//         "Received shells open stanza from %s without height attribute", from);

//   char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid");
//   char *userid_attr    = xmpp_stanza_get_attribute(stanza, "userid");

//   if (projectid_attr == NULL) {
//     open_shell_or_project(SHELL, request_attr, width_attr, height_attr, NULL, NULL);
//   } else {
//     open_shell_or_project(PROJECT, request_attr, width_attr, height_attr, projectid_attr,
//                           userid_attr);
//   }

//   return;

//   _error: ;
//     send_shells_open_response(request_attr, false, 0, false);
// }


// static void send_shells_open_response(char *request_attr, bool success, int shell_id,
//                                       bool running) {
//   if (!is_xmpp_connection_set) {
//     /* Don't send keys if not connected */
//     return;
//   }

//   werr2(request_attr == NULL, return,
//         "Trying to send shells open response without request attribute");

//   xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(message_stz, "message");
//   xmpp_stanza_set_attribute(message_stz, "to", owner);
//   xmpp_stanza_t *shells_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(shells_stz, "shells");
//   xmpp_stanza_set_ns(shells_stz, WNS);
//   xmpp_stanza_set_attribute(shells_stz, "action", "open");
//   xmpp_stanza_set_attribute(shells_stz, "request", request_attr);
//   if (success) {
//     xmpp_stanza_set_attribute(shells_stz, "response", "done");
//     char shell_id_str[8];
//     snprintf(shell_id_str, 8, "%d", shell_id);
//     xmpp_stanza_set_attribute(shells_stz, "shellid", shell_id_str);
//     xmpp_stanza_set_attribute(shells_stz, "running", running ? "true" : "false");
//   } else {
//     xmpp_stanza_set_attribute(shells_stz, "response", "error");
//   }
//   xmpp_stanza_add_child(message_stz, shells_stz);
//   xmpp_send(global_conn, message_stz);
//   xmpp_stanza_release(shells_stz);
//   xmpp_stanza_release(message_stz);
}


// static void open_shell_or_project(shell_type_t shell_type, char *request_attr,
//                                   char *width_attr, char *height_attr,
//                                   char *projectid_attr, char *userid_attr) {
//   werr2(shell_type != SHELL && shell_type != PROJECT, goto _error, "Unrecognized shell type");

//   /* Sanity checks */
//   werr2(width_attr     == NULL, return, "Trying to open shell with NULL width");
//   werr2(height_attr    == NULL, return, "Trying to open shell with NULL height");
//   if (shell_type == PROJECT) {
//     werr2(projectid_attr == NULL, return, "Trying to open project with NULL projectid");
//   }

//   if (shell_type == PROJECT) {
//     int shell_index;
//     char projectid_filepath[128];
//     snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);

//     int projectid_fd = open(projectid_filepath, O_RDONLY);
//     if (projectid_fd != -1) {
//       werr2(request_attr == NULL, return, "There should be a request for this shell");
//       werr2(userid_attr == NULL, return, "There should be an userid for this shell");

//       read(projectid_fd, &shell_index, sizeof(int));
//       close(projectid_fd);

//       werr2(shells_vector[shell_index] == NULL, return,
//             "There should be an allocated shell");

//       /* Update shell */
//       pthread_mutex_lock(&shells_lock);
//       free(shells_vector[shell_index]->request);
//       shells_vector[shell_index]->request = strdup(request_attr);
//       free(shells_vector[shell_index]->userid);
//       shells_vector[shell_index]->userid = strdup(userid_attr);
//       shells_vector[shell_index]->is_connected = true;
//       pthread_mutex_unlock(&shells_lock);

//       winfo("Project %s is running", projectid_attr);
//       send_shells_open_response(request_attr, true, shell_index, true);
//       return;
//     }
//   }

//   /* Get width and height for winsize */
//   char *endptr;

//   long width = strtol(width_attr, &endptr, 10);
//   werr2(*endptr != '\0', goto _error, "Invalid width attribute: %s", width_attr);

//   long height = strtol(height_attr, &endptr, 10);
//   werr2(*endptr != '\0', goto _error, "Invalid height attribute: %s", height_attr);

//   struct winsize ws = { height, width, 0, 0 };

//   /* Fork */
//   int fdm;
//   int pid = forkpty(&fdm, NULL, NULL, &ws);
//   wsyserr2(pid == -1, goto _error, "Forkpty failed");

//   if (pid == 0) { /* Child from forkpty */
//     int local_env_size;
//     char **local_env = build_local_env(shell_type, &local_env_size, request_attr,
//                                        projectid_attr, userid_attr);

//     char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

//     char **exec_argv;

//     if (shell_type == SHELL) {
//       chdir(home);
//       exec_argv = string_to_array((char *)shell);
//     } else if (shell_type == PROJECT) {
//       char cd_path[256];
//       snprintf(cd_path, 256, "%s/%s", build_file, projectid_attr);
//       chdir(cd_path);
//       exec_argv = string_to_array((char *)run);
//     } else {
//       werr("Unrecognized shell type");
//     }

//     execvpe(exec_argv[0], exec_argv, all_env);

//     wsyserr2(true, /* Do nothing */, "Exec failed");
//     exit(EXIT_FAILURE);
//   }

//   /* NOTE: If an error occurs after this point, the child process should be killed. */

//   /* Get an entry in the shells_vector */
//   pthread_mutex_lock(&shells_lock);

//   int shell_index = get_entry_in_shells_vector();
//   werr2(shell_index == MAX_SHELLS, goto _error, "Only %d open shells are allowed", MAX_SHELLS);

//   bool new_shell_alloc_rc;
//   if (shell_type == SHELL) {
//     new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
//                                                        request_attr, NULL, NULL);
//   } else {
//     new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
//                                                        request_attr, projectid_attr,
//                                                        userid_attr);
//   }
//   werr2(!new_shell_alloc_rc, goto _error, "Could not add new shell");

//   pthread_mutex_unlock(&shells_lock);

//   if (shell_type == PROJECT) {
//     char projectid_filepath[128];
//     snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);
//     int projectid_fd = open(projectid_filepath, O_CREAT | O_WRONLY, 0600);
//     write(projectid_fd, &shell_index, sizeof(int));
//     close(projectid_fd);

//     int open_rc = open(RUNNING_PROJECTS_PATH, O_WRONLY | O_APPEND);
//     char projectid_attr_with_colon[64];
//     sprintf(projectid_attr_with_colon, "%s:", projectid_attr);
//     write(open_rc, projectid_attr_with_colon, strlen(projectid_attr_with_colon));
//     fsync(open_rc);
//     close(open_rc);
//   }

//   /* Create new thread for read routine */
//   pthread_t rt;
//   int rc = pthread_create(&rt, NULL, &read_routine, shells_vector[shell_index]);
//   wsyserr2(rc < 0, goto _error, "Could not create thread for read routine");
//   pthread_detach(rt);

//   if (request_attr != NULL) {
//     /* Not starting a dead project */
//     send_shells_open_response(request_attr, true, shell_index, false);
//   }

//   if (shell_type == PROJECT) {
//     winfo("Project %s is now running in shell %d", projectid_attr, shell_index);
//   } else {
//     winfo("Opening shell %d", shell_index);
//   }

//   return;

//   _error:
//     werr("Failed to open new shell");
//     if (request_attr != NULL) {
//       /* Not starting a dead project */
//       send_shells_open_response(request_attr, false, 0, false);
//     }
// }


static void shells_close(const char *data) {
//   char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
//   werr2(request_attr == NULL, return,
//         "Received shells close stanza without request attribute from %s", from);

//   char *shellid_attr = xmpp_stanza_get_attribute(stanza, "shellid");
//   werr2(shellid_attr == NULL, return,
//         "Received shells close stanza without shellid attribute from %s", from);

//   char *endptr;
//   long int shellid = strtol(shellid_attr, &endptr, 10);
//   werr2(*endptr != '\0', goto _error, "Invalid shellid attribute: %s", shellid_attr);

//   werr2(shells_vector[shellid] == NULL, goto _error, "Shell %ld already closed", shellid);

//   /* Set close request or ignore it if it comes from unopened shell */
//   pthread_mutex_lock(&shells_lock);
//   free(shells_vector[shellid]->request);
//   shells_vector[shellid]->request = strdup(request_attr);
//   shells_vector[shellid]->is_connected = false;
//   pthread_mutex_unlock(&shells_lock);

//   if (xmpp_stanza_get_attribute(stanza, "background") == NULL) {
//     char screen_quit_cmd[64];
//     snprintf(screen_quit_cmd, 64, "%s %d", stop, shells_vector[shellid]->pid);
//     system(screen_quit_cmd);
//   }

//   return;

//   _error: ;
//     send_shells_close_response(request_attr, shellid_attr, NULL);
}


// static void send_shells_close_response(char *request_attr, char *shellid_attr, char *code) {
//   if (!is_xmpp_connection_set) {
//     /* Don't send keys if not connected */
//     return;
//   }

//   werr2(request_attr == NULL, return, "Trying to send close response without request attribute");
//   werr2(shellid_attr == NULL, return, "Trying to send close response without shellid attribute");

//   xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(message_stz, "message");
//   xmpp_stanza_set_attribute(message_stz, "to", owner);
//   xmpp_stanza_t *close_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(close_stz, "shells");
//   xmpp_stanza_set_ns(close_stz, WNS);
//   xmpp_stanza_set_attribute(close_stz, "request", request_attr);
//   xmpp_stanza_set_attribute(close_stz, "action", "close");
//   xmpp_stanza_set_attribute(close_stz, "shellid", shellid_attr);
//   xmpp_stanza_set_attribute(close_stz, "code", code != NULL ? code : "-1");
//   xmpp_stanza_add_child(message_stz, close_stz);
//   xmpp_send(global_conn, message_stz);
//   xmpp_stanza_release(close_stz);
//   xmpp_stanza_release(message_stz);
// }


static void shells_keys(const char *data) {
//   char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
//   werr2(request_attr == NULL, return,
//         "Received shells keys with no request attribute from %s", from);

//   char *shellid_attr = xmpp_stanza_get_attribute(stanza, "shellid");
//   werr2(shellid_attr == NULL, return,
//         "Received shells keys with no shellid attribute from %s", from);

//   char *data_str = xmpp_stanza_get_text(stanza);
//   if (data_str == NULL) {
//     /* Ignore stanza with no data */
//     return;
//   }

//   /* Decode */
//   int dec_size = strlen(data_str) * 3 / 4 + 1;
//   uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
//   werr2(decoded == NULL, return, "Could not allocate memory for decoding keys");
//   int decode_rc = base64_decode(decoded, data_str, dec_size);

//   char *endptr;
//   long int shellid = strtol(shellid_attr, &endptr, 10);
//   werr2(*endptr != '\0', return, "Invalid shellid %s", shellid_attr);

//   werr2(shells_vector[shellid] == NULL, return, "Got keys from not existent shell %ld", shellid);

//   /* Update shell request */
//   pthread_mutex_lock(&shells_lock);
//   if (shells_vector[shellid]->request != NULL) {
//     free(shells_vector[shellid]->request);
//   }
//   shells_vector[shellid]->request = strdup(request_attr);
//   pthread_mutex_unlock(&shells_lock);

//   /* Send decoded data to screen */
//   write(shells_vector[shellid]->fdm, decoded, decode_rc);
}


static void shells_status(const char *data) {
//   char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
//   werr2(request_attr == NULL, return,
//         "Received shells status with no request attribute from %s", from);

//   char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid");
//   werr2(projectid_attr == NULL, return,
//         "Received shells status with no projectid attribute from %s", from);

//   char projectid_filepath[128];
//   snprintf(projectid_filepath, 127, "/tmp/wyliodrin/%s", projectid_attr);
//   int projectid_fd = open(projectid_filepath, O_RDWR);
//   bool is_project_running = projectid_fd != -1;
//   if (is_project_running) {
//     close(projectid_fd);
//   }

//   xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(message_stz, "message");
//   xmpp_stanza_set_attribute(message_stz, "to", owner);
//   xmpp_stanza_t *shells_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(shells_stz, "shells");
//   xmpp_stanza_set_ns(shells_stz, WNS);
//   xmpp_stanza_set_attribute(shells_stz, "action", "status");
//   xmpp_stanza_set_attribute(shells_stz, "request", request_attr);
//   xmpp_stanza_set_attribute(shells_stz, "projectid", projectid_attr);
//   xmpp_stanza_set_attribute(shells_stz, "running",
//     is_project_running ? "true" : "false");

//   xmpp_stanza_add_child(message_stz, shells_stz);
//   xmpp_send(global_conn, message_stz);
//   xmpp_stanza_release(shells_stz);
//   xmpp_stanza_release(message_stz);
}


static void shells_disconnect(const char *data) {
//   char *request_attr = xmpp_stanza_get_attribute(stanza, "request");
//   werr2(request_attr == NULL, return,
//         "Received shells disconnect stanza without request attribute from %s", from);

//   char *shellid_attr = xmpp_stanza_get_attribute(stanza, "shellid");
//   werr2(shellid_attr == NULL, return,
//         "Received shells disconnect stanza without shellid attribute from %s", from);

//   char *endptr;
//   long int shellid = strtol(shellid_attr, &endptr, 10);
//   werr2(*endptr != '\0', return, "Invalid shellid attribute: %s", shellid_attr);

//   werr2(shells_vector[shellid] == NULL, return, "Shell %ld already disconnected", shellid);

//   winfo("Shell %s disconnected", shellid_attr);

//   pthread_mutex_lock(&shells_lock);
//   shells_vector[shellid]->is_connected = false;
//   pthread_mutex_unlock(&shells_lock);
}


static void shells_poweroff() {
//   if (strncmp(board, "server", 6) != 0) {
//     char cmd[64];
//     snprintf(cmd, 64, "%spoweroff", strcmp(board, "raspberrypi") == 0 ? "sudo " : "");
//     system(cmd);
//   }

//   exit(EXIT_SUCCESS);
}


// static void send_shells_keys_response(char *data_str, int data_len, int shell_id) {
//   if (!is_xmpp_connection_set) {
//     /* Don't send keys if not connected */
//     return;
//   }

//   if (!shells_vector[shell_id]->is_connected) {
//     /* Don't send keys to not connected shell */
//     return;
//   }

//   xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(message_stz, "message");
//   xmpp_stanza_set_attribute(message_stz, "to", owner);
//   xmpp_stanza_t *keys_stz = xmpp_stanza_new(global_ctx);
//   xmpp_stanza_set_name(keys_stz, "shells");
//   xmpp_stanza_set_ns(keys_stz, WNS);
//   char shell_id_str[8];
//   snprintf(shell_id_str, 8, "%d", shell_id);
//   xmpp_stanza_set_attribute(keys_stz, "shellid", shell_id_str);
//   xmpp_stanza_set_attribute(keys_stz, "action", "keys");
//   xmpp_stanza_set_attribute(keys_stz, "request", shells_vector[shell_id]->request);
//   xmpp_stanza_t *data = xmpp_stanza_new(global_ctx);

//   char *encoded_data = malloc(BASE64_SIZE(data_len) * sizeof(char));
//   wsyserr2(encoded_data == NULL, return, "Could not allocate memory for keys");
//   encoded_data = base64_encode(encoded_data, BASE64_SIZE(data_len),
//     (const unsigned char *)data_str, data_len);
//   werr2(encoded_data == NULL, return, "Could not encode keys data");

//   xmpp_stanza_set_text(data, encoded_data);
//   xmpp_stanza_add_child(keys_stz, data);
//   xmpp_stanza_add_child(message_stz, keys_stz);
//   xmpp_send(global_conn, message_stz);
//   xmpp_stanza_release(data);
//   xmpp_stanza_release(keys_stz);
//   xmpp_stanza_release(message_stz);

//   free(encoded_data);
// }


// static char **string_to_array(char *str) {
//   char **return_value = NULL;
//   int num_spaces = 0;

//   char *saveptr;
//   char *p = strtok_r(str, " ", &saveptr);
//   while (p) {
//     num_spaces++;
//     return_value = realloc(return_value, sizeof(char*) * num_spaces);
//     wsyserr2(return_value == NULL, return NULL, "Could not reallocate memory");

//     return_value[num_spaces-1] = p;

//     p = strtok_r(NULL, " ", &saveptr);
//   }

//   num_spaces++;
//   return_value = realloc(return_value, sizeof(char*) * num_spaces);
//   wsyserr2(return_value == NULL, return NULL, "Could not reallocate memory");
//   return_value[num_spaces-1] = NULL;

//   return return_value;
// }


// static char **concatenation_of_local_and_user_env(char **local_env, int local_env_size) {
//   int environ_size = 0;
//   while (environ[environ_size]) {
//     environ_size++;
//   }

//   char **all_env = malloc((environ_size + local_env_size) * sizeof(char *));
//   wsyserr2(all_env == NULL, return NULL, "Could not allocate memory for environment variables");
//   memcpy(all_env, environ, environ_size * sizeof(char *));
//   memcpy(all_env + environ_size, local_env, local_env_size * sizeof(char *));

//   return all_env;
// }


// static int get_entry_in_shells_vector() {
//   int shell_index;

//   for (shell_index = 0; shell_index < MAX_SHELLS; shell_index++) {
//     if (shells_vector[shell_index] == NULL) {
//       break;
//     }
//   }

//   return shell_index;
// }


// static bool allocate_memory_for_new_shell(int shell_index, int pid, int fdm,
//                                           long int width, long int height,
//                                           char *request_attr, char *projectid_attr,
//                                           char *userid_attr) {
//   werr2(shells_vector[shell_index] != NULL, return false,
//         "Trying to allocate memory for new shell with index %d, but shell is still in use",
//         shell_index);

//   shells_vector[shell_index] = malloc(sizeof(shell_t));
//   wsyserr2(shells_vector[shell_index] == NULL, return false,
//            "Allocation of memory for new shell failed");

//   shells_vector[shell_index]->id             = shell_index;
//   shells_vector[shell_index]->pid            = pid;
//   shells_vector[shell_index]->fdm            = fdm;
//   shells_vector[shell_index]->width          = width;
//   shells_vector[shell_index]->height         = height;
//   if (request_attr != NULL) {
//     shells_vector[shell_index]->is_connected = true;
//     shells_vector[shell_index]->request      = strdup(request_attr);
//   } else {
//     shells_vector[shell_index]->is_connected = false;
//     shells_vector[shell_index]->request      = NULL;
//   }
//   if (projectid_attr != NULL) {
//     shells_vector[shell_index]->projectid    = strdup(projectid_attr);
//   } else {
//     shells_vector[shell_index]->projectid    = NULL;
//   }
//   if (userid_attr != NULL) {
//     shells_vector[shell_index]->userid       = strdup(userid_attr);
//   } else {
//     shells_vector[shell_index]->userid       = NULL;
//   }

//   return true;
// }


// static char **build_local_env(shell_type_t shell_type, int *num_env,
//                               char *request_attr, char *projectid_attr, char *userid_attr) {
//   if (shell_type == SHELL) {
//     return build_local_env_for_shell(num_env, NULL, NULL, NULL);
//   } else if (shell_type == PROJECT) {
//     return build_local_env_for_project(num_env, request_attr, projectid_attr, userid_attr);
//   } else {
//     werr("Unrecognized shell type");
//     return NULL;
//   }
// }


// static char **build_local_env_for_shell(int *num_env, char *request_attr, char *projectid_attr,
//                                         char *userid_attr) {
//   *num_env = 5;

//   char **local_env = malloc((*num_env) * sizeof(char *));
//   int i;
//   for (i = 0; i < *num_env - 1; i++) {
//     local_env[i] = malloc(64 * sizeof(char));
//   }
//   local_env[*num_env - 1] = NULL;

//   snprintf(local_env[0], 64, "wyliodrin_board=%s", board);
//   snprintf(local_env[1], 64, "wyliodrin_server=%d.%d", WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR);
//   snprintf(local_env[2], 64, "HOME=%s", home);
//   snprintf(local_env[3], 64, "TERM=xterm");

//   return local_env;
// }


// static char **build_local_env_for_project(int *num_env, char *request_attr, char *projectid_attr,
//                                           char *userid_attr) {
//   *num_env = 7;

//   char **local_env = malloc((*num_env) * sizeof(char *));
//   int i;
//   for (i = 0; i < *num_env - 1; i++) {
//     local_env[i] = malloc(64 * sizeof(char));
//   }
//   local_env[*num_env - 1] = NULL;

//   snprintf(local_env[0], 64, "wyliodrin_project=%s", projectid_attr);
//   snprintf(local_env[1], 64, "wyliodrin_userid=%s", userid_attr != NULL ? userid_attr : "null");
//   snprintf(local_env[2], 64, "wyliodrin_session=%s", request_attr != NULL ? request_attr : "null");
//   snprintf(local_env[3], 64, "wyliodrin_board=%s", board);
//   snprintf(local_env[4], 64, "wyliodrin_jid=%s", jid);
//   snprintf(local_env[5], 64, "wyliodrin_server=%d.%d", WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR);

//   return local_env;
// }


// static void remove_project_id_from_running_projects(char *projectid_attr) {
//   char cmd[256];
//   snprintf(cmd, 256, "sed -i -e 's/%s://g' %s", projectid_attr, RUNNING_PROJECTS_PATH);
//   system(cmd);
// }


// static void *read_routine(void *args) {
//   shell_t *shell = (shell_t *)args;

//   int num_reads = 0;

//   char buf[BUFSIZE];
//   while (true) {
//     int read_rc = read(shell->fdm, buf, sizeof(buf));
//     if (read_rc > 0) {
//       if (shell->request != NULL && shell->is_connected) {
//         /* Send keys only to active shells */
//         send_shells_keys_response(buf, read_rc, shell->id);
//       }
//       num_reads++;
//       if (num_reads == 1000) {
//         usleep(100000);
//         num_reads = 0;
//       }
//     } else if (read_rc < 0) {
//       char shellid_str[8];
//       snprintf(shellid_str, 8, "%d", shell->id);

//       int status;
//       waitpid(shell->pid, &status, 0);

//       werr2(WIFSIGNALED(status), /* Do nothing */, "Shell closed on signal %d", WTERMSIG(status));

//       char status_str[8];
//       snprintf(status_str, 8, "%d", WEXITSTATUS(status));

//       /* Send close stanza */
//       if (shell->request != NULL) {
//         /* Project died before attaching to shell */
//         send_shells_close_response(shell->request, shellid_str, status_str);
//       }

//       bool restart_project = false;
//       char width_str[8];
//       char height_str[8];
//       char *projectid;
//       char *userid;
//       if (!shell->is_connected && shell->projectid != NULL &&
//           ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
//            (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV))) {
//         /* Restart project */
//         if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
//           winfo("Restarting project %s because it exited with code %d while running in background",
//                 shell->projectid, WEXITSTATUS(status));
//         } else {
//           winfo("Restarting project %s because it received SIGSEGV while running in background",
//                 shell->projectid);
//         }

//         snprintf(width_str, 8, "%ld", shell->width);
//         snprintf(height_str, 8, "%ld", shell->height);
//         projectid = strdup(shell->projectid);
//         userid = strdup(shell->userid);
//         restart_project = true;
//       }

//       pthread_mutex_lock(&shells_lock);
//       if (shell->request != NULL) {
//         free(shell->request);
//       }
//       if (shell->projectid != NULL) {
//         remove_project_id_from_running_projects(shell->projectid);

//         char projectid_path[64];
//         snprintf(projectid_path, 64, "/tmp/wyliodrin/%s", shell->projectid);
//         remove(projectid_path);

//         free(shell->projectid);
//         free(shell->userid);
//       }
//       close(shell->fdm);
//       winfo("Shell %d closed", shell->id);
//       shells_vector[shell->id] = NULL;
//       free(shell);
//       pthread_mutex_unlock(&shells_lock);

//       if (restart_project) {
//         open_shell_or_project(PROJECT, NULL, width_str, height_str,
//                               projectid, userid);
//         free(projectid);
//         free(userid);
//       }

//       return NULL;
//     }
//   }
// }

/*************************************************************************************************/



#endif /* SHELLS */
