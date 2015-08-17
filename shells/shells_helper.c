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


#include "../winternals/winternals.h" /* logs and errs */
#include "shells.h"
#include "shells_helper.h"
#include "../wxmpp/wxmpp.h"

#define BUFSIZE (1 * 1024) /* Size of buffer used for reading */

extern const char *owner_str; /* owner_str from init.c */

static pthread_mutex_t projectid_lock; /* shells mutex */

void *read_thread(void *args) {
  shell_t *shell = (shell_t *)args;
  int fdm = shell->fdm;

  /* Get data from PTY and send it to server */
  char buf[BUFSIZE];
  while (1) {
    int rc_int = read(fdm, buf, sizeof(buf));
    if (rc_int > 0) {
      send_shells_keys_response(shell->conn, (void *)shell->ctx, buf, rc_int, shell->id);
      usleep(10000);
    } else if (rc_int < 0) {
      char shellid_str[4];
      snprintf(shellid_str, 4, "%d", shell->id);

      int status;
      waitpid(shell->pid, &status, 0);
      char exit_status[4];
      snprintf(exit_status, 4, "%d",  WEXITSTATUS(status));

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
        char projectid_path[64];
        snprintf(projectid_path, 64, "/tmp/wyliodrin/%s", shell->projectid);
        free(shell->projectid);
        shell->projectid = NULL;

        int remove_rc = remove(projectid_path);
        if (remove_rc != 0) {
          werr("WTAF");
          perror("remove");
        }
      }
      pthread_mutex_unlock(&projectid_lock);

      return NULL;
    }
  }
}
