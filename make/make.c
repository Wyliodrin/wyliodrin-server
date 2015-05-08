/**************************************************************************************************
 * Make module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifdef MAKE

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

extern const char *owner_str; /* owner_str from init.c */

void init_make() {

}

typedef struct {
  xmpp_conn_t *conn;
  void *userdata;
  char *projectid_attr;
  char *request_attr;
} thread_arg;

void *fork_thread(void *args) {
  thread_arg *arg = (thread_arg *)args;

  xmpp_conn_t *conn = arg->conn;
  void *userdata = arg->userdata;
  char *projectid_attr = arg->projectid_attr;
  char *request_attr = arg->request_attr;

  /* Copy */
  pid_t pid;
  int status;

  pid = fork();

  wfatal(pid == -1, "fork returned -1");

  if (pid == 0) {
    char src[4 + strlen(projectid_attr) + 1];
    sprintf(src, "mnt/%s", projectid_attr);

    char *args[] = {"cp", "-r", src, "build", NULL};
    execvp(args[0], args);

    wfatal(true,"execvp failed");
  }

  waitpid(pid, &status, 0);
  wfatal(!WIFEXITED(status), "cp failed");

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

  char *text = "I'm trying my best...";
  char *encoded_data = (char *)malloc(BASE64_SIZE(strlen(text)));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(strlen(text)), 
    (const unsigned char *)text, strlen(text));

  xmpp_stanza_t *data_stz = xmpp_stanza_new(ctx); /* make stanza */
  xmpp_stanza_set_text(data_stz, encoded_data);

  xmpp_stanza_add_child(make_stz, data_stz);
  xmpp_stanza_add_child(message_stz, make_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(message_stz);

  /* Send done */
  message_stz = xmpp_stanza_new(ctx); /* message stanza with make */
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", owner_str);
  make_stz = xmpp_stanza_new(ctx); /* make stanza */
  xmpp_stanza_set_name(make_stz, "make");
  xmpp_stanza_set_ns(make_stz, WNS);
  xmpp_stanza_set_attribute(make_stz, "action", "build");
  xmpp_stanza_set_attribute(make_stz, "response", "done");
  xmpp_stanza_set_attribute(make_stz, "request", request_attr);
  xmpp_stanza_set_attribute(make_stz, "code", "0");

  xmpp_stanza_add_child(message_stz, make_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(message_stz);

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

    int rc = pthread_create(&ft, NULL, fork_thread, arg); /* Read rc */
    wsyserr(rc < 0, "pthread_create");
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
