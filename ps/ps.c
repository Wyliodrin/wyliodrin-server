/**************************************************************************************************
 * PS module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifdef PS

#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define BUF_SIZE 1024

#include "ps.h"
#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"
#include "../shells/shells.h"

bool keep_sending = false;

extern const char *owner; /* owner from init.c */
extern const char *stop;

static void ps_kill(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);
static void ps_send(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);
static void ps_stop(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

typedef struct {
  xmpp_conn_t *conn;
  void *userdata;
} thread_arg;

static void *send_tasks_routine(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;

  while (keep_sending == true) {
    DIR *dp;
    struct dirent *ep;
    char fn[64];
    char buf[BUF_SIZE];
    int fd, rc;

    char pid[32];
    char comm[32];
    char vmSize[32];

    dp = opendir ("/proc");
    if (dp != NULL) {
      xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

      xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
      xmpp_stanza_set_name(message, "message");
      xmpp_stanza_set_attribute(message, "to", owner);

      xmpp_stanza_t *info = xmpp_stanza_new(ctx); /* info stanza */
      xmpp_stanza_set_name(info, "info");
      xmpp_stanza_set_ns(info, WNS);
      xmpp_stanza_set_attribute(info, "data", "ps");

      while ((ep = readdir(dp))) {
        if (isdigit(ep->d_name[0])) {
          /* stat */
          snprintf(fn, 63, "/proc/%s/stat", ep->d_name);
          fd = open(fn, O_RDONLY);
          if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
          }

          memset(buf, 0, BUF_SIZE);
          rc = read(fd, buf, BUF_SIZE);
          if (rc == -1) {
            perror("read");
          }

          sscanf(buf, "%s %s", pid, comm);

          //fprintf(stderr, "%s %d ", comm, pid);

          rc = close(fd);
          if (rc == -1) {
            perror("close");
            exit(EXIT_FAILURE);
          }

          /* statm */
          snprintf(fn, 63, "/proc/%s/statm", ep->d_name);
          fd = open(fn, O_RDONLY);
          if (fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
          }

          memset(buf, 0, BUF_SIZE);
          rc = read(fd, buf, BUF_SIZE);
          if (rc == -1) {
            perror("read");
          }

          sscanf(buf, "%s", vmSize);

          rc = close(fd);
          if (rc == -1) {
            perror("close");
            exit(EXIT_FAILURE);
          }

          xmpp_stanza_t *ps = xmpp_stanza_new(ctx); /* info stanza */
          xmpp_stanza_set_name(ps, "ps");
          xmpp_stanza_set_attribute(ps, "pid", pid);
          xmpp_stanza_set_attribute(ps, "cpu", "50");
          xmpp_stanza_set_attribute(ps, "mem", vmSize);

          xmpp_stanza_set_attribute(ps, "name", comm);


          xmpp_stanza_add_child(info, ps);
        }
      }
      (void) closedir (dp);

      xmpp_stanza_add_child(message, info);
      xmpp_send(conn, message);
      xmpp_stanza_release(message);
    } else {
      perror ("Couldn't open the directory");
    }

    sleep(5);
  }
  return NULL;
}

/**
 * Parse ps commands
 */
void ps(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
        xmpp_conn_t *const conn, void *const userdata) {
  wlog("ps()");

  char *action_attr = xmpp_stanza_get_attribute(stanza, "action"); /* action attribute */
  if (action_attr == NULL) {
    werr("Error while getting action attribute");
    return;
  }

  if(strncasecmp(action_attr, "kill", 3) == 0) {
    ps_kill(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "send", 4) == 0) {
    ps_send(stanza, conn, userdata);
  } else if(strncasecmp(action_attr, "stop", 4) == 0) {
    ps_stop(stanza, conn, userdata);
  } else {
    werr("Unknown action attribute: %s", action_attr);
  }

  wlog("Return from ps()");
}

static void ps_kill(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("ps_kill()");

  char *pid_attr = xmpp_stanza_get_attribute(stanza, "pid");
  if (pid_attr == NULL) {
    werr("Got kill command without pid");
    return;
  }
  char cmd[64];
  snprintf(cmd, 63, "%s %s", stop, pid_attr);
  system(cmd);
}

static void ps_send(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("ps_send()");

  if (keep_sending == true) {
    /* Ignore */
    return;
  }

  keep_sending = true;

  pthread_t stt; /* send tasks thread */
  thread_arg *arg = malloc(sizeof(thread_arg));
  wsyserr(arg == NULL, "malloc");
  arg->conn = conn;
  arg->userdata = userdata;
  int rc = pthread_create(&stt, NULL, &(send_tasks_routine), arg);
  if (rc < 0) {
    werr("SYSERR pthread_create");
    perror("pthread_create");
    return;
  }
  pthread_detach(stt);

  wlog("Return from ps_send()");
}

static void ps_stop(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("ps_stop()");

  keep_sending = false;
}

#endif /* PS */
