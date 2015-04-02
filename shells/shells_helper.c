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

int openpty(int *fdm, int *fds) {
  int rc;

  *fdm = posix_openpt(O_RDWR);
  if (*fdm < 0) {
    werr("SYSERR posix_openpt");
    perror("posix_openpt");
    return -1;
  }

  rc = grantpt(*fdm);
  if (rc != 0) {
    werr("SYSERR grantpt");
    perror("grantpt");
    return -1;
  }

  rc = unlockpt(*fdm);
  if (rc != 0) {
    werr("SYSERR unlockpt");
    perror("unlockpt");
    return -1;
  }

  *fds = open(ptsname(*fdm), O_RDWR);
  if (*fds < 0) {
    werr("SYSERR open");
    perror("open");
    return -1;    
  }

  return 0;
}

void *read_thread(void *args) {
  int rc;
  char buf[150];
  shell_t *shell = (shell_t *)args;
  int fdm = shell->fdm;

  while (1) {
    rc = read(fdm, buf, sizeof(buf));
    if (rc > 0) {
      // Send data with keys
      wlog("Send data via keys: %s", buf);
    } else if (rc < 0) {
      wlog("SYSERR read");
      perror("read");
    }
  }
}
