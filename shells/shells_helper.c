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
      wlog("SYSERR read");
      perror("read");
    }
  }
}
