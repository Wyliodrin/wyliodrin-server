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

int main(int argc, char *argv[]) {
  int fdm, fds;
  int rc;
  char input[150];

  // Check arguments
  if (argc <= 1) {
    fprintf(stderr, "Usage: %s program_name [parameters]\n", argv[0]);
    return -1;
  }

  fdm = posix_openpt(O_RDWR);
  if (fdm < 0) {
    fprintf(stderr, "Error %d on posix_openpt()\n", errno);
    return -2;
  }

  rc = grantpt(fdm);
  if (rc != 0) {
    fprintf(stderr, "Error %d on grantpt()\n", errno);
    return -3;
  }

  rc = unlockpt(fdm);
  if (rc != 0) {
    fprintf(stderr, "Error %d on unlockpt()\n", errno);
    return -4;
  }

  // Open the slave side ot the PTY
  fds = open(ptsname(fdm), O_RDWR);

  // Create the child process
  if (fork() != 0) { /* Parent */
    fd_set fd_in;

    // Close the slave side of the PTY
    close(fds);

    while (1) {
      // Wait for data from standard input and master side of PTY
      FD_ZERO(&fd_in);
      FD_SET(0, &fd_in);
      FD_SET(fdm, &fd_in);

      rc = select(fdm + 1, &fd_in, NULL, NULL, NULL);
      switch(rc) {
      case -1 :
        fprintf(stderr, "Error %d on select()\n", errno);
        return -5;

        default : {
          // If data on standard input
          if (FD_ISSET(0, &fd_in)) {
            rc = read(0, input, sizeof(input));
            if (rc > 0) {
              // Send data on the master side of PTY
              write(fdm, input, rc);
            } else {
              if (rc < 0) {
                fprintf(stderr, "Error %d on read standard input\n", errno);
                exit(1);
              }
            }
          }

          // If data on master side of PTY
          if (FD_ISSET(fdm, &fd_in)) {
            rc = read(fdm, input, sizeof(input));
            if (rc > 0) {
              // Send data on standard output
              write(1, input, rc);
            } else {
              if (rc < 0) {
                fprintf(stderr, "Error %d on read master PTY\n", errno);
                exit(1);
              }
            }
          }
        }
      } // End switch
    } // End while
  } else { /* Child */
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

    // The slave side of the PTY becomes the standard input and outputs of the child process
    close(0); // Close standard input (current terminal)
    close(1); // Close standard output (current terminal)
    close(2); // Close standard error (current terminal)

    dup(fds); // PTY becomes standard input (0)
    dup(fds); // PTY becomes standard output (1)
    dup(fds); // PTY becomes standard error (2)

    // Now the original file descriptor is useless
    close(fds);

    // Make the current process a new session leader
    setsid();

    // As the child is a session leader, set the controlling terminal to be the slave side of the PTY
    // (Mandatory for programs like the shell to make them manage correctly their outputs)
    ioctl(0, TIOCSCTTY, 1);

    // Execution of the program
    {
      char **child_av;
      int i;

      // Build the command line
      child_av = (char **)malloc(argc * sizeof(char *));
      for (i = 1; i < argc; i ++) {
        child_av[i - 1] = strdup(argv[i]);
      }
      child_av[i - 1] = NULL;
      rc = execvp(child_av[0], child_av);
    }

    // if Error...
    return -7;
  }

  return 0;
} // main