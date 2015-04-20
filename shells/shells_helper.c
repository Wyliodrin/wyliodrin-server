/**************************************************************************************************
 * Shells helper
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
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

#include <strophe.h> /* Strophe XMPP stuff */

#include "../winternals/winternals.h" /* logs and errs */
#include "shells.h"
#include "shells_helper.h"
#include "../wxmpp/wxmpp.h"

#define BUFSIZE (1 * 1024) /* 1 KB */

void *read_thread(void *args) {
  int rc;
  char buf[BUFSIZE];
  shell_t *shell = (shell_t *)args;
  int fdm = shell->fdm;

  /* Get data from PTY and send it to server */
  while (1) {
    rc = read(fdm, buf, sizeof(buf));
    if (rc > 0) {
      send_shells_keys_response(shell->conn, (void *)shell->ctx, buf, rc, shell->id);
    } else if (rc < 0) {

      char shellid_str[4];
      sprintf(shellid_str, "%d", shell->id);

      /* Send close stanza */
      xmpp_stanza_t *message_stz = xmpp_stanza_new(shell->ctx); /* message stanza */
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner_str);

      xmpp_stanza_t *close_stz = xmpp_stanza_new(shell->ctx); /* close stanza */
      xmpp_stanza_set_name(close_stz, "shells");
      xmpp_stanza_set_ns(close_stz, WNS);
      if (shell->close_request != -1) {  
        char close_request_str[4];
        sprintf(close_request_str, "%d", shell->close_request);
        xmpp_stanza_set_attribute(close_stz, "request", close_request_str);
      }
      xmpp_stanza_set_attribute(close_stz, "action", "close");
      xmpp_stanza_set_attribute(close_stz, "shellid", shellid_str);
      xmpp_stanza_set_attribute(close_stz, "code", "0");

      xmpp_stanza_add_child(message_stz, close_stz);

      xmpp_send(shell->conn, message_stz);

      xmpp_stanza_release(message_stz);

      return NULL;
    }
  }
}
