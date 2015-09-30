/**************************************************************************************************
 * Make module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifdef MAKE

#include <stdio.h>   /* snprintf    */
#include <string.h>  /* strcmp     */
#include <strophe.h> /* xmpp stuff */
#include <stdlib.h>  /* malloc     */

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <pthread.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* WNS */
#include "../base64/base64.h"

#include "make.h"

#define BUFSIZE (1 * 1024) /* 1 KB */

extern const char *owner_str;      /* owner_str from wtalk.c */
extern const char *mount_file_str; /* mount file */
extern const char *build_file_str; /* build file */
extern const char *board_str;      /* board_str from wtalk.c */
extern bool is_fuse_available;     /* from wtalk.c */

void init_make() { }

typedef struct {
  xmpp_conn_t *conn;
  void *userdata;
  char *projectid_attr;
  char *request_attr;
  char *address_attr;
  int fd;
} thread_arg;

void *out_read_thread(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;
  char *request_attr = arg->request_attr;
  int fd = arg->fd;

  int rc;
  char buf[BUFSIZE];

  while(1) {
    rc = read(fd, buf, BUFSIZE);
    if (rc > 0) {
      /* Send Working */
      xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

      xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx); /* message stanza with make */
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner_str);
      xmpp_stanza_t *make_stz = xmpp_stanza_new(ctx); /* make stanza */
      xmpp_stanza_set_name(make_stz, "make");
      xmpp_stanza_set_ns(make_stz, WNS);
      xmpp_stanza_set_attribute(make_stz, "action", "build");
      xmpp_stanza_set_attribute(make_stz, "response", "working");
      xmpp_stanza_set_attribute(make_stz, "request", request_attr);
      xmpp_stanza_set_attribute(make_stz, "source", "stdout");

      char *encoded_data = (char *)malloc(BASE64_SIZE(rc));
      encoded_data = base64_encode(encoded_data, BASE64_SIZE(rc),
        (const unsigned char *)buf, rc);

      xmpp_stanza_t *data_stz = xmpp_stanza_new(ctx); /* make stanza */
      xmpp_stanza_set_text(data_stz, encoded_data);

      xmpp_stanza_add_child(make_stz, data_stz);
      xmpp_stanza_add_child(message_stz, make_stz);
      xmpp_send(conn, message_stz);
      xmpp_stanza_release(data_stz);
      xmpp_stanza_release(make_stz);
      xmpp_stanza_release(message_stz);
    } else if (rc < 0) {
      return NULL;
    }
  }
}

void *err_read_thread(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;
  char *request_attr = arg->request_attr;
  int fd = arg->fd;

  int rc;
  char buf[BUFSIZE];

  while(1) {
    rc = read(fd, buf, BUFSIZE);
    if (rc > 0) {
      /* Send Working */
      xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

      xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx); /* message stanza with make */
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner_str);
      xmpp_stanza_t *make_stz = xmpp_stanza_new(ctx); /* make stanza */
      xmpp_stanza_set_name(make_stz, "make");
      xmpp_stanza_set_ns(make_stz, WNS);
      xmpp_stanza_set_attribute(make_stz, "action", "build");
      xmpp_stanza_set_attribute(make_stz, "response", "working");
      xmpp_stanza_set_attribute(make_stz, "request", request_attr);
      xmpp_stanza_set_attribute(make_stz, "source", "stderr");

      char *encoded_data = (char *)malloc(BASE64_SIZE(rc));
      encoded_data = base64_encode(encoded_data, BASE64_SIZE(rc),
        (const unsigned char *)buf, rc);

      xmpp_stanza_t *data_stz = xmpp_stanza_new(ctx); /* make stanza */
      xmpp_stanza_set_text(data_stz, encoded_data);

      xmpp_stanza_add_child(make_stz, data_stz);
      xmpp_stanza_add_child(message_stz, make_stz);
      xmpp_send(conn, message_stz);
      xmpp_stanza_release(make_stz);
      xmpp_stanza_release(data_stz);
      xmpp_stanza_release(message_stz);
    } else if (rc < 0) {
      return NULL;
    }
  }
}

void *status_read_thread(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;
  char *request_attr = arg->request_attr;
  int fd = arg->fd;

  int rc;
  char buf[BUFSIZE];

  while(1) {
    memset(buf, 0, BUFSIZE);
    rc = read(fd, buf, BUFSIZE);
    if (rc > 0) {
      /* Send done */
      xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

      xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx); /* message stanza with make */
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner_str);
      xmpp_stanza_t *make_stz = xmpp_stanza_new(ctx); /* make stanza */
      xmpp_stanza_set_name(make_stz, "make");
      xmpp_stanza_set_ns(make_stz, WNS);
      xmpp_stanza_set_attribute(make_stz, "action", "build");
      xmpp_stanza_set_attribute(make_stz, "response", "done");
      xmpp_stanza_set_attribute(make_stz, "request", request_attr);
      xmpp_stanza_set_attribute(make_stz, "code", buf);

      xmpp_stanza_add_child(message_stz, make_stz);
      xmpp_send(conn, message_stz);
      xmpp_stanza_release(make_stz);
      xmpp_stanza_release(message_stz);
    } else if (rc < 0) {
      return NULL;
    }
  }
}

void *fork_thread(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;
  char *projectid_attr = arg->projectid_attr;
  char *request_attr = arg->request_attr;

  /* Copy */
  pid_t pid;
  int status;
  int rc;
  int out_fd[2];
  int err_fd[2];
  int status_fd[2];

  rc = pipe(out_fd);
  wsyserr(rc != 0, "pipe");

  rc = pipe(err_fd);
  wsyserr(rc != 0, "pipe");

  rc = pipe(status_fd);
  wsyserr(rc != 0, "pipe");

  /* Create read threads */
  pthread_t out_rt, err_rt, status_rt; /* Output, error and status read threads */

  thread_arg *out_arg = (thread_arg *)malloc(sizeof(thread_arg));
  out_arg->conn = conn;
  out_arg->userdata = userdata;
  out_arg->request_attr = strdup(request_attr);
  out_arg->fd = out_fd[0];

  thread_arg *err_arg = (thread_arg *)malloc(sizeof(thread_arg));
  err_arg->conn = conn;
  err_arg->userdata = userdata;
  err_arg->request_attr = strdup(request_attr);
  err_arg->fd = err_fd[0];

  thread_arg *status_arg = (thread_arg *)malloc(sizeof(thread_arg));
  status_arg->conn = conn;
  status_arg->userdata = userdata;
  status_arg->request_attr = strdup(request_attr);
  status_arg->fd = status_fd[0];

  rc = pthread_create(&out_rt, NULL, out_read_thread, out_arg);
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(out_rt);

  rc = pthread_create(&err_rt, NULL, err_read_thread, err_arg);
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(err_rt);

  rc = pthread_create(&status_rt, NULL, status_read_thread, status_arg);
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(status_rt);

  pid = fork();
  wsyserr(pid == -1, "fork");

  if (pid == 0) {
    char cmd[1024];
    if (!is_fuse_available) {
      snprintf(cmd, 1023, "cd %s && curl -L -k %s > %s && tar xf %s &&"
        "rm -rf %s && cd %s/%s && make -f Makefile.%s",
          build_file_str,
          arg->address_attr,
          strrchr(arg->address_attr, '/') + 1,
          strrchr(arg->address_attr, '/') + 1,
          strrchr(arg->address_attr, '/') + 1,
          build_file_str, projectid_attr,
          board_str);
    } else {
      snprintf(cmd, 1023, "cp -rf %s/%s %s && cd %s/%s && make -f Makefile.%s",
        mount_file_str, projectid_attr, build_file_str,
        build_file_str, projectid_attr,
        board_str);
    }

    rc = dup2(out_fd[1], STDOUT_FILENO); /* Redirect output to write end of out_fd */
    wsyserr(rc < 0, "dup2");
    rc = dup2(err_fd[1], STDERR_FILENO); /* Redirect error to write end of err_fd */
    wsyserr(rc < 0, "dup2");

    rc = system((const char *)cmd);
    wsyserr(rc == -1, "system");

    char system_exitstatus_str[8];
    snprintf(system_exitstatus_str, 7, "%d", rc);

    rc = write(status_fd[1], system_exitstatus_str, strlen(system_exitstatus_str));

    /* Close STDOUT and STDERR */
    /* This will cause the read threads to finish their execution */
    rc = close(STDOUT_FILENO);
    wsyserr(rc != 0, "close");
    rc = close(STDERR_FILENO);
    wsyserr(rc != 0, "close");
    rc = close(status_fd[1]);
    wsyserr(rc != 0, "close");

    exit(EXIT_SUCCESS);
  }

  waitpid(pid, &status, 0);
  if (!WIFEXITED(status)) {
    werr("make command failed");
  }

  close(out_fd[0]);
  close(out_fd[1]);
  close(err_fd[0]);
  close(err_fd[1]);
  close(status_fd[0]);
  close(status_fd[1]);

  return NULL;
}

void make(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
          xmpp_conn_t *const conn, void *const userdata) {
  wlog("make()");

  /* Get action attribute */
  char *action_attr = xmpp_stanza_get_attribute(stanza, "action"); /* action attribute */
  if (action_attr == NULL) {
    werr("No action attribute in make stanza");
    return;
  }

  /* Treat build action */
  if (strcmp(action_attr, "build") == 0) {
    /* Get projectid request */
    char *projectid_attr = xmpp_stanza_get_attribute(stanza, "projectid"); /* projectid attribute */
    if (projectid_attr == NULL) {
      werr("No projectid attribute in make stanza");
      return;
    }

    /* Get request attribute */
    char *request_attr = xmpp_stanza_get_attribute(stanza, "request"); /* request attribute */
    if (request_attr == NULL) {
      werr("No request attribute in make stanza");
    }

    pthread_t ft; /* Fork thread */

    thread_arg *arg = (thread_arg *)malloc(sizeof(thread_arg));
    arg->conn = conn;
    arg->userdata = userdata;
    arg->projectid_attr = strdup(projectid_attr);
    arg->request_attr = strdup(request_attr);
    if (!is_fuse_available) {
      char *address_attr = xmpp_stanza_get_attribute(stanza, "address"); /* address attribute */
      if (address_attr == NULL) {
        werr("No address attribute in make stanza");
      }
      arg->address_attr = strdup(address_attr);
    } else {
      arg->address_attr = NULL;
    }

    int rc = pthread_create(&ft, NULL, fork_thread, arg); /* Read rc */
    wsyserr(rc < 0, "pthread_create");
    pthread_detach(ft);
  }

  /* Treat close action */
  else if (strcmp(action_attr, "close") == 0) {
    wlog("closing make...");
  }

  /* Unknown action */
  else {
    werr("Unknown action: %s", action_attr);
  }

  wlog("Return from make()");
}

#endif /* MAKE */