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

int main(int argc, char *argv[]) {
  int fdm;
  int rc;
  int pid;
  pthread_t t;

  struct winsize w = {24, 80, 0, 0};

  // Create the child process
  pid = forkpty(&fdm, NULL, NULL, &w);
  if (pid < 0) {
    fprintf(stderr, "Could not fork\n");
    return -1;
  }

  if (pid != 0) { /* Parent */
    rc = pthread_create(&t, NULL, &(parent_thread), (void *)&fdm);
    if (rc < 0) {
      fprintf(stderr, "Could not create thread\n");
      return -1;
    }

    write(fdm, "ls\n", 3);
  }

  else {
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
