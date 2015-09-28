/**************************************************************************************************
 * Shells helper
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#define __USE_BSD
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <strophe.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pty.h>


#include "../winternals/winternals.h" /* logs and errs */
#include "shells.h"
#include "shells_helper.h"
#include "../wxmpp/wxmpp.h"
#include "../wtalk.h"

#define BUFSIZE (1 * 1024) /* Size of buffer used for reading */

extern const char *build_file_str; /* build_file_str */
extern const char *owner_str; /* owner_str from wtalk.c */
extern const char *board_str; /* board name */
extern const char *jid_str;   /* jid */

static pthread_mutex_t projectid_lock; /* shells mutex */

extern char **environ;
int execvpe(const char *file, char *const argv[], char *const envp[]);

static void remove_project_id_from_running_projects(char *projectid) {
  char cmd[256];

  snprintf(cmd, 256, "sed -i -e 's/%s://g' %s", projectid, RUNNING_PROJECTS_PATH);
  system(cmd);
}

void *read_thread(void *args) {
  shell_t *shell = (shell_t *)args;
  int fdm = shell->fdm;

  /* Get data from PTY and send it to server */
  char buf[BUFSIZE];
  while (1) {
    int rc_int = read(fdm, buf, sizeof(buf));
    if (rc_int > 0) {
      send_shells_keys_response(buf, rc_int, shell->id);
      usleep(10000);
    } else if (rc_int < 0) {
      char shellid_str[4];
      snprintf(shellid_str, 4, "%d", shell->id);

      int status;
      pid_t waitpid_rc = waitpid(shell->pid, &status, 0);
      char exit_status[4];
      snprintf(exit_status, 4, "%d",  WEXITSTATUS(status));

      if (waitpid_rc == -1) {
        werr("waitpid error");
      }
      if (shell->close_request == -1 && WIFEXITED(status) == 0) {
        /* This calls for a restart */
        struct winsize ws = {shell->height, shell->width, 0, 0};
        int fdm;
        int pid = forkpty(&fdm, NULL, NULL, &ws);
        wfatal(pid == -1, "forkpty");

        if (pid == 0) { /* Child*/
          char cd_path[256];
          snprintf(cd_path, 256, "%s/%s", build_file_str, shell->projectid);
          int chdir_rc = chdir(cd_path);
          wsyserr(chdir_rc == -1, "chdir");

          char makefile_name[64];
          snprintf(makefile_name, 64, "Makefile.%s", board_str);

          char *make_run[] = {"make", "-f", makefile_name, "run", NULL};

          char wyliodrin_project_env[64];
          snprintf(wyliodrin_project_env, 64, "wyliodrin_project=%s", shell->projectid);

          char wyliodrin_userid_env[64];
          snprintf(wyliodrin_userid_env, 64, "wyliodrin_userid=%s", shell->userid);

          char wyliodrin_session_env[64];
          snprintf(wyliodrin_session_env, 64, "wyliodrin_session=%s", shell->request_attr);

          char wyliodrin_board_env[64];
          snprintf(wyliodrin_board_env, 64, "wyliodrin_board=%s", board_str);

          char wyliodrin_jid_env[64];
          snprintf(wyliodrin_jid_env, 64, "wyliodrin_jid=%s", jid_str);

          char home_env[] = "HOME=/wyliodrin";
          char term_env[] = "TERM=xterm";

          #ifdef USEMSGPACK
            char wyliodrin_usemsgpack_env[64];
            snprintf(wyliodrin_usemsgpack_env, 64, "wyliodrin_usemsgpack=1");
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

        /* Parent from forkpty */
        /* Update shell */
        shell->pid = pid;
        shell->fdm = fdm;

        pthread_t rt; /* Read thread */
        int pthread_create_rc = pthread_create(&rt, NULL, &(read_thread), shell);
        wfatal(pthread_create_rc < 0, "pthread_create");
        pthread_detach(rt);

        return NULL;
      }

      /* Send close stanza */
      xmpp_stanza_t *message_stz = xmpp_stanza_new(shell->ctx); /* message stanza */
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner_str);
      xmpp_stanza_t *close_stz = xmpp_stanza_new(shell->ctx); /* close stanza */
      xmpp_stanza_set_name(close_stz, "shells");
      xmpp_stanza_set_ns(close_stz, WNS);
      if (shell->close_request != -1) {
        char close_request_str[4];
        snprintf(close_request_str, 4, "%d", shell->close_request);
        xmpp_stanza_set_attribute(close_stz, "request", close_request_str);
      }
      xmpp_stanza_set_attribute(close_stz, "action", "close");
      xmpp_stanza_set_attribute(close_stz, "shellid", shellid_str);
      xmpp_stanza_set_attribute(close_stz, "code", exit_status);
      xmpp_stanza_add_child(message_stz, close_stz);
      xmpp_send(shell->conn, message_stz);
      xmpp_stanza_release(close_stz);
      xmpp_stanza_release(message_stz);

      pthread_mutex_lock(&projectid_lock);
      if (shell->projectid != NULL) {
        remove_project_id_from_running_projects(shell->projectid);

        char projectid_path[64];
        snprintf(projectid_path, 64, "/tmp/wyliodrin/%s", shell->projectid);
        free(shell->projectid);
        shell->projectid = NULL;

        int remove_rc = remove(projectid_path);
        if (remove_rc != 0) {
          werr("Could not remove %s", projectid_path);
        }
      }
      pthread_mutex_unlock(&projectid_lock);

      return NULL;
    }
  }
}
