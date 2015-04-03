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
#include <pty.h>

#define BUFSIZE (1 * 1024) /* 1 KB */

void *parent_thread(void *args) {
  int rc;
  char input[150];
  int fdm = *((int *)args);

  int fd_out = open("output.txt", O_WRONLY | O_TRUNC | O_CREAT, 0664);
  if (fd_out < 0) {
    perror("open");
    return NULL;
  }

  while (1) {
    rc = read(fdm, input, sizeof(input));
    if (rc > 0) {
      // Send data on output file
      write(fd_out, input, rc);
    } else {
      if (rc < 0) {
        fprintf(stderr, "Error %d on read master PTY\n", errno);
        exit(1);
      }
    }
  }
}

// int openpty(int *fdm, int *fds) {
//   int rc;

//   *fdm = posix_openpt(O_RDWR);
//   if (*fdm < 0) {
//     fprintf(stderr, "Error %d on posix_openpt()\n", errno);
//     return -1;
//   }

//   rc = grantpt(*fdm);
//   if (rc != 0) {
//     fprintf(stderr, "Error %d on grantpt()\n", errno);
//     return -1;
//   }

//   rc = unlockpt(*fdm);
//   if (rc != 0) {
//     fprintf(stderr, "Error %d on unlockpt()\n", errno);
//     return -1;
//   }

//   // Open the slave side ot the PTY
//   *fds = open(ptsname(*fdm), O_RDWR);

//   return 0;
// }

int main(int argc, char *argv[]) {
  int fdm, fds;
  int rc;
  int pid;
  pthread_t t;

  // rc = openpty(&fdm, &fds);
  // if (rc < 0) {
  //   fprintf(stderr, "Could not openpty\n");
  //   return -1;
  // }

  struct winsize w = {24, 80, 0, 0};
  if(openpty(&fdm, &fds, NULL, NULL, &w) < 0)
  {
    return -1;
  }

  // Create the child process
  pid = fork();
  if (pid < 0) {
    fprintf(stderr, "Could not fork\n");
    return -1;
  }

  if (pid != 0) { /* Parent */
    close(fds);

    rc = pthread_create(&t, NULL, &(parent_thread), (void *)&fdm);
    if (rc < 0) {
      fprintf(stderr, "Could not create thread\n");
      return -1;
    }

    write(fdm, "whoami\n", 7);
    write(fdm, "ls\n", 3);
  }

  else { /* Child */
    struct termios slave_orig_term_settings; // Saved terminal settings
    struct termios new_term_settings; // Current terminal settings

    // Close the master side of the PTY
    close(fdm);

    // Save the defaults parameters of the slave side of the PTY
    rc = tcgetattr(fds, &slave_orig_term_settings);

    // Set RAW mode on slave side of PTY
    new_term_settings = slave_orig_term_settings;
    cfmakeraw (&new_term_settings);
    tcsetattr (fds, TCSANOW, &new_term_settings);

    dup2(fds, 0);
    dup2(fds, 1);
    dup2(fds, 2);

    // Now the original file descriptor is useless
    close(fds);

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    // Execution of the program
    char *args[] = {"bash", NULL};
    rc = execvp(args[0], args);
    
    /* Error */
    fprintf(stderr, "Could not execvp\n");
    return -1;
  }

  if (pthread_join(t, NULL)) {
    fprintf(stderr, "Could not join\n");
    return -1;
  }

  return 0;
}
