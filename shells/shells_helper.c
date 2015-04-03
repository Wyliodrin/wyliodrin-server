/**************************************************************************************************
 * Shells helper
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

#define BUFSIZE = (1 * 1024) /* 1 KB */

void *read_thread(void *args) {
  int rc;
  char buf[150];
  shell_t *shell = (shell_t *)args;
  int fdm = shell->fdm;

  int fd_out = open("outputlog.txt", O_WRONLY | O_TRUNC | O_CREAT, 0664);
  if (fd_out < 0) {
    perror("open");
    return NULL;
  }

  while (1) {
    rc = read(fdm, buf, sizeof(buf));
    if (rc > 0) {
      // Send data with keys
      write(fd_out, buf, strlen(buf));
      send_shells_keys_response(shell->conn, (void *)shell->ctx, buf, shell->id);
    } else if (rc < 0) {
      wlog("SYSERR read");
      perror("read");
    }
  }
}
