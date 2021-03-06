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
#include "../../libds/ds.h"           /* hashmap */
#include "../wredis/wredis.h"         /* redis */
#include "../base64/base64.h"         /* encode decode */
// #include "../wtalk.h"                 /* RUNNING_PROJECTS_PATH */

#include "shells.h"                   /* shells module api */
#include "wyliodrin_hypervisor_config.h"  /* version */

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

extern const char *jid;
extern const char *home;
extern const char *owner;
extern const char *board;
extern const char *shell;
extern const char *run;
extern const char *stop;
extern const char *poweroff;
extern const char *build_file;

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
static void shells_open(hashmap_p hm);

/**
 * Build and sent shells open response
 */
static void send_shells_open_response(char *request_attr, bool success, int shell_id,
                                      bool running);

/**
 * Open shell or project
 */
static void open_shell_or_project(shell_type_t shell_type, char *request_attr,
                                  char *width_attr, char *height_attr,
                                  char *projectid_attr, char *userid_attr);

/**
 * Close shell
 */
static void shells_close(hashmap_p hm);

/**
 * Send shells close response
 */
static void send_shells_close_response(char *request_attr, char *shellid_attr, char *code);

/**
 * Get keys.
 */
static void shells_keys(hashmap_p hm);

/**
 * Status of project (running or not).
 */
static void shells_status(hashmap_p hm);

/**
 * Disconnect shell.
 */
static void shells_disconnect(hashmap_p hm);

/**
 * Resize shell.
 */
static void shells_resize(hashmap_p hm);

/**
 * Poweroff board.
 */
static void shells_poweroff();

/**
 * Send shells keys response.
 */
static void send_shells_keys_response(char *data_str, int data_len, int shell_id);

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

/**
 * Generic build local env
 */
static char **build_local_env(shell_type_t shell_type, int *num_env,
                              char *request_attr, char *projectid_attr, char *userid_attr);

/**
 * Build local env for shell.
 */
static char **build_local_env_for_shell(int *num_env, char *request_attr, char *projectid_attr,
                                        char *userid_attr);

/**
 * Build local env for project.
 */
static char **build_local_env_for_project(int *num_env, char *request_attr, char *projectid_attr,
                                          char *userid_attr);

/**
 * Remove project id from running projects file.
 */
// static void remove_project_id_from_running_projects(char *projectid_attr);

/**
 * Routine for shell or project.
 */
static void *read_routine(void *args);

/************************************************************************************************/



/*** ENVIRONMENT *********************************************************************************/

extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);

/************************************************************************************************/


/*** API IMPLEMENTATION **************************************************************************/

void init_shells() {
  winfo("Shell initialization");

  winfo("Creating /tmp/wyliodrin");
  int mkdir_rc = mkdir("/tmp/wyliodrin", 0700);
  if (mkdir_rc != 0) {
    winfo("/tmp/wyliodrin exists. Removing its contents.");
    system("rm -rf /tmp/wyliodrin/*");
  }

  int i;
  for (i = 0; i < MAX_SHELLS; i++) {
    shells_vector[i] = NULL;
  }
}


void shells(hashmap_p hm) {
  char *action = (char *)hashmap_get(hm, "a");
  werr2(action == NULL, return, "There is no action in shells open stanza");

  if (strncasecmp(action, "o", 1) == 0) {
    shells_open(hm);
  } else if (strncasecmp(action, "c", 1) == 0) {
    shells_close(hm);
  } else if (strncasecmp(action, "k", 1) == 0) {
    shells_keys(hm);
  } else if (strncasecmp(action, "s", 1) == 0) {
    shells_status(hm);
  } else if (strncasecmp(action, "p", 1) == 0) {
    shells_poweroff();
  } else if (strncasecmp(action, "d", 1) == 0) {
    shells_disconnect(hm);
  } else if (strncasecmp(action, "r", 1) == 0) {
    shells_resize(hm);
  } else {
    werr("Received shells stanza with unknown action attribute %s", action);
  }
}


// void start_dead_projects() {
//   // FILE *fp = fopen(RUNNING_PROJECTS_PATH, "r");
//   // werr2(fp == NULL, return, "Could no open %s", RUNNING_PROJECTS_PATH);

//   // char projectid[128];
//   // while (fscanf(fp, "%[^:]:", projectid) != EOF) {
//   //   if (strlen(projectid) > 0) {
//   //     remove_project_id_from_running_projects(projectid);
//   //     winfo("Starting project %s", projectid);
//   //     open_shell_or_project(PROJECT, NULL, DEFAULT_WIDTH, DEFAULT_HEIGHT, projectid, NULL);
//   //   }
//   // }

//   // fclose(fp);
// }

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void shells_open(hashmap_p hm) {
  /* Sanity checks */
  char *request_attr = (char *)hashmap_get(hm, "r");
  werr2(request_attr == NULL, goto _error,
        "Received shells open without request");

  char *width_attr = (char *)hashmap_get(hm, "w");
  werr2(width_attr == NULL, goto _error,
        "Received shells open stanza without width");

  char *height_attr = (char *)hashmap_get(hm, "h");
  werr2(height_attr == NULL, goto _error,
        "Received shells open stanza without height");

  char *projectid_attr = (char *)hashmap_get(hm, "p");
  char *userid_attr    = (char *)hashmap_get(hm, "u");

  if (projectid_attr == NULL) {
    winfo("Open shell request");
    open_shell_or_project(SHELL, request_attr, width_attr, height_attr, NULL, NULL);
  } else {
    winfo("Open project %s request", projectid_attr);
    open_shell_or_project(PROJECT, request_attr, width_attr, height_attr,
          projectid_attr, userid_attr);
  }

  return;

  _error: ;
    send_shells_open_response(request_attr, false, 0, false);
}


static void send_shells_open_response(char *request_attr, bool success, int shell_id,
                                      bool running) {
  werr2(request_attr == NULL, return,
        "Trying to send shells open response without request attribute");

  // xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
  // xmpp_stanza_set_name(message_stz, "message");
  // xmpp_stanza_set_attribute(message_stz, "to", owner);
  // xmpp_stanza_t *shells_stz = xmpp_stanza_new(global_ctx);
  // xmpp_stanza_set_name(shells_stz, "shells");
  // xmpp_stanza_set_ns(shells_stz, WNS);
  // xmpp_stanza_set_attribute(shells_stz, "action", "open");
  // xmpp_stanza_set_attribute(shells_stz, "request", request_attr);
  // if (success) {
  //   xmpp_stanza_set_attribute(shells_stz, "response", "done");
  //   char shell_id_str[8];
  //   snprintf(shell_id_str, 8, "%d", shell_id);
  //   xmpp_stanza_set_attribute(shells_stz, "shellid", shell_id_str);
  //   xmpp_stanza_set_attribute(shells_stz, "running", running ? "true" : "false");
  // } else {
  //   xmpp_stanza_set_attribute(shells_stz, "response", "error");
  // }
  // xmpp_stanza_add_child(message_stz, shells_stz);
  // xmpp_send(global_conn, message_stz);
  // xmpp_stanza_release(shells_stz);
  // xmpp_stanza_release(message_stz);

  char msgpack_buf[256];

  cmp_ctx_t cmp;
  cmp_init(&cmp, msgpack_buf, 256);

  uint32_t map_size = 3;
  if (success) {
    map_size += 2;
  }

  werr2(!cmp_write_map(&cmp, 2 * map_size),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Action */
  werr2(!cmp_write_str(&cmp, "a", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, "o", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Request */
  werr2(!cmp_write_str(&cmp, "r", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, request_attr, strlen(request_attr)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Response */
  werr2(!cmp_write_str(&cmp, "re", 2),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  if (success) {
    werr2(!cmp_write_str(&cmp, "done", 4),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Shellid */
    char shellid_str[8];
    snprintf(shellid_str, 8, "%d", shell_id);

    werr2(!cmp_write_str(&cmp, "s", 1),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, shellid_str, strlen(shellid_str)),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Running */
    werr2(!cmp_write_str(&cmp, "ru", 2),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, running ? "true" : "false", strlen(running ? "true" : "false")),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
  } else {
    werr2(!cmp_write_str(&cmp, "error", 5),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
  }

  publish(msgpack_buf, cmp.writer_offset);
}


static void open_shell_or_project(shell_type_t shell_type, char *request_attr,
                                  char *width_attr, char *height_attr,
                                  char *projectid_attr, char *userid_attr) {
  /* Sanity checks */
  werr2(shell_type != SHELL && shell_type != PROJECT, goto _error, "Unrecognized shell type");
  werr2(width_attr     == NULL, return, "Trying to open shell with NULL width");
  werr2(height_attr    == NULL, return, "Trying to open shell with NULL height");
  if (shell_type == PROJECT) {
    werr2(projectid_attr == NULL, return, "Trying to open project with NULL projectid");
  }

  if (shell_type == PROJECT) {
    int shell_index;
    char projectid_filepath[128];
    snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);

    int projectid_fd = open(projectid_filepath, O_RDONLY);
    if (projectid_fd != -1) {
      werr2(request_attr == NULL, return, "There should be a request for this shell");
      werr2(userid_attr == NULL, return, "There should be an userid for this shell");

      read(projectid_fd, &shell_index, sizeof(int));
      close(projectid_fd);

      werr2(shells_vector[shell_index] == NULL, return,
            "There should be an allocated shell");

      /* Update shell */
      pthread_mutex_lock(&shells_lock);
      free(shells_vector[shell_index]->request);
      shells_vector[shell_index]->request = strdup(request_attr);
      free(shells_vector[shell_index]->userid);
      shells_vector[shell_index]->userid = strdup(userid_attr);
      shells_vector[shell_index]->is_connected = true;
      pthread_mutex_unlock(&shells_lock);

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
    int local_env_size;
    char **local_env = build_local_env(shell_type, &local_env_size, request_attr,
                                       projectid_attr, userid_attr);

    char **all_env = concatenation_of_local_and_user_env(local_env, local_env_size);

    char **exec_argv;

    if (shell_type == SHELL) {
      chdir(home);
      exec_argv = string_to_array((char *)shell);
    } else if (shell_type == PROJECT) {
      char cd_path[256];
      snprintf(cd_path, 256, "%s/%s", build_file, projectid_attr);
      chdir(cd_path);
      exec_argv = string_to_array((char *)run);
    } else {
      werr("Unrecognized shell type");
    }

    execvpe(exec_argv[0], exec_argv, all_env);

    wsyserr2(true, /* Do nothing */, "Exec failed");
    exit(EXIT_FAILURE);
  }

  /* NOTE: If an error occurs after this point, the child process should be killed. */

  /* Get an entry in the shells_vector */
  pthread_mutex_lock(&shells_lock);

  int shell_index = get_entry_in_shells_vector();
  werr2(shell_index == MAX_SHELLS, goto _error, "Only %d open shells are allowed", MAX_SHELLS);

  bool new_shell_alloc_rc;
  if (shell_type == SHELL) {
    new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
                                                       request_attr, NULL, NULL);
  } else {
    new_shell_alloc_rc = allocate_memory_for_new_shell(shell_index, pid, fdm, width, height,
                                                       request_attr, projectid_attr,
                                                       userid_attr);
  }
  werr2(!new_shell_alloc_rc, goto _error, "Could not add new shell");

  pthread_mutex_unlock(&shells_lock);

  if (shell_type == PROJECT) {
    char projectid_filepath[128];
    snprintf(projectid_filepath, 128, "/tmp/wyliodrin/%s", projectid_attr);
    int projectid_fd = open(projectid_filepath, O_CREAT | O_WRONLY, 0600);
    write(projectid_fd, &shell_index, sizeof(int));
    close(projectid_fd);

    // int open_rc = open(RUNNING_PROJECTS_PATH, O_WRONLY | O_APPEND);
    // char projectid_attr_with_colon[64];
    // sprintf(projectid_attr_with_colon, "%s:", projectid_attr);
    // write(open_rc, projectid_attr_with_colon, strlen(projectid_attr_with_colon));
    // fsync(open_rc);
    // close(open_rc);
  }

  /* Create new thread for read routine */
  pthread_t rt;
  int rc = pthread_create(&rt, NULL, &read_routine, shells_vector[shell_index]);
  wsyserr2(rc < 0, goto _error, "Could not create thread for read routine");
  pthread_detach(rt);

  if (request_attr != NULL) {
    /* Not starting a dead project */
    send_shells_open_response(request_attr, true, shell_index, false);
  }

  if (shell_type == SHELL) {
    winfo("Successfully opened shell %d with pid %d", shell_index, pid);
  } else {
    winfo("Successfully opened project %s inside shell %d with pid %d", projectid_attr, shell_index, pid);
  }

  return;

  _error:
    werr("Failed to open new %s", shell_type == SHELL ? "shell" : "project");
    if (request_attr != NULL) {
      /* Not starting a dead project */
      send_shells_open_response(request_attr, false, 0, false);
    }
}


static void shells_close(hashmap_p hm) {
  char *request_attr = (char *)hashmap_get(hm, "r");
  werr2(request_attr == NULL, return,
        "Received shells close stanza without request attribute");

  char *shellid_attr = (char *)hashmap_get(hm, "s");
  werr2(shellid_attr == NULL, return,
        "Received shells close stanza without shellid attribute");

  char *endptr;
  long int shellid = strtol(shellid_attr, &endptr, 10);
  werr2(*endptr != '\0', goto _error, "Invalid shellid attribute: %s", shellid_attr);

  werr2(shells_vector[shellid] == NULL, goto _error, "Shell %ld already closed", shellid);

  /* Set close request or ignore it if it comes from unopened shell */
  pthread_mutex_lock(&shells_lock);
  free(shells_vector[shellid]->request);
  shells_vector[shellid]->request = strdup(request_attr);
  shells_vector[shellid]->is_connected = false;
  pthread_mutex_unlock(&shells_lock);

  if ((char *)hashmap_get(hm, "background") == NULL) {
    char screen_quit_cmd[64];
    snprintf(screen_quit_cmd, 64, "%s %d", stop, shells_vector[shellid]->pid);
    system(screen_quit_cmd);
  }

  return;

  _error: ;
    send_shells_close_response(request_attr, shellid_attr, NULL);
}


static void send_shells_close_response(char *request_attr, char *shellid_attr, char *code) {
  werr2(request_attr == NULL, return, "Trying to send close response without request attribute");
  werr2(shellid_attr == NULL, return, "Trying to send close response without shellid attribute");

  /* Init msgpack */
  cmp_ctx_t cmp;
  char msgpack_buf[128];
  cmp_init(&cmp, msgpack_buf, 128);

  /* Init msgpack map */
  uint32_t map_size = 1 + /* action  */
                      1 + /* request */
                      1 + /* shellid */
                      1;  /* code    */
  werr2(!cmp_write_map(&cmp, 2 * map_size),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write action */
  werr2(!cmp_write_str(&cmp, "a", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, "c", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write request */
  werr2(!cmp_write_str(&cmp, "r", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, request_attr, strlen(request_attr)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));


  /* Write shellid */
  werr2(!cmp_write_str(&cmp, "s", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, shellid_attr, strlen(shellid_attr)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write code */
  werr2(!cmp_write_str(&cmp, "c", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, code != NULL ? code : "-1", strlen(code != NULL ? code : "-1")),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Send msgpack map via redis */
  publish(msgpack_buf, cmp.writer_offset);
}


static void shells_keys(hashmap_p hm) {
  char *request_attr = (char *)hashmap_get(hm, "r");
  werr2(request_attr == NULL, return,
        "Received shells keys with no request attribute");

  char *shellid_attr = (char *)hashmap_get(hm, "s");
  werr2(shellid_attr == NULL, return,
        "Received shells keys with no shellid attribute");

  char *data_str = (char *)hashmap_get(hm, "t");
  if (data_str[0] == 0) {
    /* Ignore stanza with no data */
    cmp_ctx_t cmp;
    char msgpack_buf[64];
    cmp_init(&cmp, msgpack_buf, 64);

    /* Init msgpack map */
    uint32_t map_size = 1 + /* action  */
                        1 + /* request */
                        1 + /* shellid */
                        1;  /* text    */
    werr2(!cmp_write_map(&cmp, 2 * map_size),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Write action */
    werr2(!cmp_write_str(&cmp, "a", 1),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, "k", 1),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Write request */
    werr2(!cmp_write_str(&cmp, "r", 7),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, request_attr, strlen(request_attr)),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Write shellid */
    werr2(!cmp_write_str(&cmp, "s", 1),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, shellid_attr, strlen(shellid_attr)),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Write text */
    werr2(!cmp_write_str(&cmp, "t", 1),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));
    werr2(!cmp_write_str(&cmp, "", 0),
          return,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    /* Send msgpack map via redis */
    publish(msgpack_buf, cmp.writer_offset);

    return;
  }

  /* Decode */
  // int dec_size = strlen(data_str) * 3 / 4 + 1;
  // uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
  // werr2(decoded == NULL, return, "Could not allocate memory for decoding keys");
  // int decode_rc = base64_decode(decoded, data_str, dec_size);
  char *decoded = data_str;
  int decode_rc = strlen(data_str);

  char *endptr;
  long int shellid = strtol(shellid_attr, &endptr, 10);
  werr2(*endptr != '\0', return, "Invalid shellid %s", shellid_attr);

  werr2(shells_vector[shellid] == NULL, return, "Got keys from not existent shell %ld", shellid);

  /* Update shell request */
  pthread_mutex_lock(&shells_lock);
  if (shells_vector[shellid]->request != NULL) {
    free(shells_vector[shellid]->request);
  }
  shells_vector[shellid]->request = strdup(request_attr);
  pthread_mutex_unlock(&shells_lock);

  /* Send decoded data to screen */
  write(shells_vector[shellid]->fdm, decoded, decode_rc);
}


static void shells_status(hashmap_p hm) {
  char *request_attr = (char *)hashmap_get(hm, "r");
  werr2(request_attr == NULL, return,
        "Received shells status with no request attribute");

  char *projectid_attr = (char *)hashmap_get(hm, "p");
  werr2(projectid_attr == NULL, return,
        "Received shells status with no projectid attribute");

  char projectid_filepath[128];
  snprintf(projectid_filepath, 127, "/tmp/wyliodrin/%s", projectid_attr);
  int projectid_fd = open(projectid_filepath, O_RDWR);
  bool is_project_running = projectid_fd != -1;
  if (is_project_running) {
    close(projectid_fd);
  }

//   xmpp_stanza_set_attribute(shells_stz, "action", "status");
//   xmpp_stanza_set_attribute(shells_stz, "request", request_attr);
//   xmpp_stanza_set_attribute(shells_stz, "projectid", projectid_attr);
//   xmpp_stanza_set_attribute(shells_stz, "running",
//     is_project_running ? "true" : "false");

  /* Init msgpack */
  cmp_ctx_t cmp;
  char msgpack_buf[128];
  cmp_init(&cmp, msgpack_buf, 128);

  /* Init msgpack map */
  uint32_t map_size = 1 + /* action    */
                      1 + /* request   */
                      1 + /* projectid */
                      1;  /* running   */
  werr2(!cmp_write_map(&cmp, 2 * map_size),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write action */
  werr2(!cmp_write_str(&cmp, "a", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, "s", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write request */
  werr2(!cmp_write_str(&cmp, "r", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, request_attr, strlen(request_attr)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));


  /* Write projectid */
  werr2(!cmp_write_str(&cmp, "p", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, projectid_attr, strlen(projectid_attr)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write running */
  werr2(!cmp_write_str(&cmp, "ru", 2),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, is_project_running ? "true" : "false",
                       strlen(is_project_running ? "true" : "false")),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Send msgpack map via redis */
  publish(msgpack_buf, cmp.writer_offset);
}


static void shells_disconnect(hashmap_p hm) {
  char *request_attr = (char *)hashmap_get(hm, "r");
  werr2(request_attr == NULL, return,
        "Received shells disconnect stanza without request attribute");

  char *shellid_attr = (char *)hashmap_get(hm, "s");
  werr2(shellid_attr == NULL, return,
        "Received shells disconnect stanza without shellid attribute");

  char *endptr;
  long int shellid = strtol(shellid_attr, &endptr, 10);
  werr2(*endptr != '\0', return, "Invalid shellid attribute: %s", shellid_attr);

  werr2(shells_vector[shellid] == NULL, return, "Shell %ld already disconnected", shellid);

  winfo("Shell %s disconnected", shellid_attr);

  pthread_mutex_lock(&shells_lock);
  shells_vector[shellid]->is_connected = false;
  pthread_mutex_unlock(&shells_lock);
}


static void shells_resize(hashmap_p hm) {
  char *shellid_attr = (char *)hashmap_get(hm, "s");
  werr2(shellid_attr == NULL, return,
        "Received shells close stanza without shellid attribute");

  char *endptr;
  long shellid = strtol(shellid_attr, &endptr, 10);
  werr2(*endptr != '\0', return, "Invalid shellid attribute: %s", shellid_attr);

  werr2(shells_vector[shellid] == NULL, return, "Shell %ld not open", shellid);

  char *width_attr = (char *)hashmap_get(hm, "w");
  werr2(width_attr == NULL, return,
        "Received shells resize stanza without width");

  char *height_attr = (char *)hashmap_get(hm, "h");
  werr2(height_attr == NULL, return,
        "Received shells resize stanza without height");

  long width = strtol(width_attr, &endptr, 10);
  werr2(*endptr != '\0', return, "Invalid width attribute: %s", width_attr);

  long height = strtol(height_attr, &endptr, 10);
  werr2(*endptr != '\0', return, "Invalid height attribute: %s", height_attr);

  struct winsize ws = { height, width, 0, 0 };

  ioctl(shells_vector[shellid]->fdm, TIOCSWINSZ, &ws);

  winfo("Shell %ld resized to %ld x %ld", shellid, width, height);
}


static void shells_poweroff() {
  system(poweroff);
  exit(EXIT_SUCCESS);
}


static void send_shells_keys_response(char *data_str, int data_len, int shell_id) {
  /* Encode data */
  // char *encoded_data = malloc(BASE64_SIZE(data_len) * sizeof(char));
  // wsyserr2(encoded_data == NULL, return, "Could not allocate memory for keys");
  // encoded_data = base64_encode(encoded_data, BASE64_SIZE(data_len),
  //   (const unsigned char *)data_str, data_len);
  // werr2(encoded_data == NULL, return, "Could not encode keys data");
  char *encoded_data = data_str;

  /* Init msgpack */
  cmp_ctx_t cmp;
  char msgpack_buf[64 + BASE64_SIZE(data_len)];
  cmp_init(&cmp, msgpack_buf, 64 + BASE64_SIZE(data_len));

  /* Init msgpack map */
  uint32_t map_size = 1 + /* action  */
                      1 + /* request */
                      1 + /* shellid */
                      1;  /* text    */
  werr2(!cmp_write_map(&cmp, 2 * map_size),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write action */
  werr2(!cmp_write_str(&cmp, "a", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, "k", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write request */
  werr2(!cmp_write_str(&cmp, "r", 7),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, shells_vector[shell_id]->request, strlen(shells_vector[shell_id]->request)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));


  /* Write shellid */
  char shell_id_str[8];
  snprintf(shell_id_str, 8, "%d", shell_id);
  werr2(!cmp_write_str(&cmp, "s", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, shell_id_str, strlen(shell_id_str)),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Write text */
  werr2(!cmp_write_str(&cmp, "t", 1),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));
  werr2(!cmp_write_str(&cmp, encoded_data, /* strlen(encoded_data) */ data_len),
        return,
        "cmp_write_map error: %s", cmp_strerror(&cmp));

  /* Send msgpack map via redis */
  publish(msgpack_buf, cmp.writer_offset);

  /* Clean */
  // free(encoded_data);
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
  if (request_attr != NULL) {
    shells_vector[shell_index]->is_connected = true;
    shells_vector[shell_index]->request      = strdup(request_attr);
  } else {
    shells_vector[shell_index]->is_connected = false;
    shells_vector[shell_index]->request      = NULL;
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

  return true;
}


static char **build_local_env(shell_type_t shell_type, int *num_env,
                              char *request_attr, char *projectid_attr, char *userid_attr) {
  if (shell_type == SHELL) {
    return build_local_env_for_shell(num_env, NULL, NULL, NULL);
  } else if (shell_type == PROJECT) {
    return build_local_env_for_project(num_env, request_attr, projectid_attr, userid_attr);
  } else {
    werr("Unrecognized shell type");
    return NULL;
  }
}


static char **build_local_env_for_shell(int *num_env, char *request_attr, char *projectid_attr,
                                        char *userid_attr) {
  *num_env = 5;

  char **local_env = malloc((*num_env) * sizeof(char *));
  int i;
  for (i = 0; i < *num_env - 1; i++) {
    local_env[i] = malloc(64 * sizeof(char));
  }
  local_env[*num_env - 1] = NULL;

  snprintf(local_env[0], 64, "wyliodrin_board=%s", board);
  snprintf(local_env[1], 64, "wyliodrin_server=%d.%d",
           WYLIODRIN_HYPERVISOR_VERSION_MAJOR,
           WYLIODRIN_HYPERVISOR_VERSION_MINOR);
  snprintf(local_env[2], 64, "HOME=%s", home);
  snprintf(local_env[3], 64, "TERM=xterm");

  return local_env;
}


static char **build_local_env_for_project(int *num_env, char *request_attr, char *projectid_attr,
                                          char *userid_attr) {
  *num_env = 8;

  char **local_env = malloc((*num_env) * sizeof(char *));
  int i;
  for (i = 0; i < *num_env - 1; i++) {
    local_env[i] = malloc(64 * sizeof(char));
  }
  local_env[*num_env - 1] = NULL;

  snprintf(local_env[0], 64, "wyliodrin_project=%s", projectid_attr);
  snprintf(local_env[1], 64, "wyliodrin_userid=%s", userid_attr != NULL ? userid_attr : "null");
  snprintf(local_env[2], 64, "wyliodrin_session=%s", request_attr != NULL ? request_attr : "null");
  snprintf(local_env[3], 64, "wyliodrin_board=%s", board);
  snprintf(local_env[4], 64, "wyliodrin_jid=%s", jid);
  snprintf(local_env[5], 64, "wyliodrin_server=%d.%d",
           WYLIODRIN_HYPERVISOR_VERSION_MAJOR, WYLIODRIN_HYPERVISOR_VERSION_MINOR);
  snprintf(local_env[6], 64, "HOME=%s", home);

  return local_env;
}


// static void remove_project_id_from_running_projects(char *projectid_attr) {
//   char cmd[256];
//   snprintf(cmd, 256, "sed -i -e 's/%s://g' %s", projectid_attr, RUNNING_PROJECTS_PATH);
//   system(cmd);
// }


static void *read_routine(void *args) {
  shell_t *shell = (shell_t *)args;

  int num_reads = 0;

  char buf[BUFSIZE];
  while (true) {
    int read_rc = read(shell->fdm, buf, sizeof(buf));
    if (read_rc > 0) {
      if (shell->request != NULL && shell->is_connected) {
        /* Send keys only to active shells */
        send_shells_keys_response(buf, read_rc, shell->id);
      }
      num_reads++;
      if (num_reads == 1000) {
        usleep(100000);
        num_reads = 0;
      }
    } else if (read_rc < 0) {
      char shellid_str[8];
      snprintf(shellid_str, 8, "%d", shell->id);

      int status;
      waitpid(shell->pid, &status, 0);

      werr2(WIFSIGNALED(status), /* Do nothing */, "Shell closed on signal %d", WTERMSIG(status));

      char status_str[8];
      snprintf(status_str, 8, "%d", WEXITSTATUS(status));

      /* Send close stanza */
      if (shell->request != NULL) {
        /* Project died before attaching to shell */
        send_shells_close_response(shell->request, shellid_str, status_str);
      }

      bool restart_project = false;
      char width_str[8];
      char height_str[8];
      char *projectid;
      char *userid;
      if (!shell->is_connected && shell->projectid != NULL &&
          ((WIFEXITED(status) && WEXITSTATUS(status) != 0) ||
           (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV))) {
        /* Restart project */
        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
          winfo("Restarting project %s because it exited with code %d while running in background",
                shell->projectid, WEXITSTATUS(status));
        } else {
          winfo("Restarting project %s because it received SIGSEGV while running in background",
                shell->projectid);
        }

        snprintf(width_str, 8, "%ld", shell->width);
        snprintf(height_str, 8, "%ld", shell->height);
        projectid = strdup(shell->projectid);
        userid = strdup(shell->userid);
        restart_project = true;
      }

      pthread_mutex_lock(&shells_lock);
      if (shell->request != NULL) {
        free(shell->request);
      }
      if (shell->projectid != NULL) {
        // remove_project_id_from_running_projects(shell->projectid);

        char projectid_path[64];
        snprintf(projectid_path, 64, "/tmp/wyliodrin/%s", shell->projectid);
        remove(projectid_path);

        free(shell->projectid);
        free(shell->userid);
      }
      close(shell->fdm);
      winfo("Shell %d closed", shell->id);
      shells_vector[shell->id] = NULL;
      free(shell);
      pthread_mutex_unlock(&shells_lock);

      if (restart_project) {
        open_shell_or_project(PROJECT, NULL, width_str, height_str,
                              projectid, userid);
        free(projectid);
        free(userid);
      }

      return NULL;
    }
  }
}

/*************************************************************************************************/



#endif /* SHELLS */
