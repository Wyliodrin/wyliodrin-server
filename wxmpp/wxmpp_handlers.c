/**************************************************************************************************
 * XMPP handlers
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */
#include <string.h>  /* strlen */
#include <pthread.h> /* mutex and cond */

#include "../winternals/winternals.h" /* logs and errs*/
#include "../libds/ds.h"              /* hashmap */
#include "wxmpp.h"                    /* tag_function and WNS */
#include "wxmpp_handlers.h"           /* handlers api */
#include "../shells/shells.h"
#include "../files/files.h"

bool_t is_user_online = false;

extern pthread_mutex_t mutex; /* mutex      */
extern pthread_cond_t  cond;  /* condition  */

/* Condition signals */
extern bool_t signal_attr;
extern bool_t signal_list;
extern bool_t signal_read;
extern bool_t signal_fail;

/* Wyliodrin connection handler */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata) {
  wlog("wconn_handler(...)");

  if (status == XMPP_CONN_CONNECT) {
    wlog("Connection success");

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
    
    /* Add ping handler */
    xmpp_handler_add(conn, wping_handler, "urn:xmpp:ping", "iq", "get", ctx);

    /* Add presence handler */
    xmpp_handler_add(conn, wpresence_handler, NULL, "presence", NULL, ctx);

    /* Add wyliodrin handler */
    xmpp_handler_add(conn, wyliodrin_handler, WNS, "message", NULL, ctx);

    /* Send presence */
    xmpp_stanza_t *pres = xmpp_stanza_new(ctx); /* Presence stanza */
    xmpp_stanza_set_name(pres, "presence");
    xmpp_stanza_t *priority = xmpp_stanza_new (ctx); /* Priority */
    xmpp_stanza_set_name(priority, "priority");
    xmpp_stanza_add_child(pres, priority);
    xmpp_stanza_t *value = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(value, "50");
    xmpp_stanza_add_child(priority, value);
    xmpp_stanza_release(value);
    xmpp_stanza_release(priority);
    xmpp_send(conn, pres);
    xmpp_stanza_release(pres);

    /* Send subscribe */
    xmpp_stanza_t *subscribe = xmpp_stanza_new(ctx); /* Subscribe stanza */
    xmpp_stanza_set_name(subscribe, "presence");
    xmpp_stanza_set_attribute(subscribe, "to", owner_str);
    xmpp_stanza_set_type(subscribe, "subscribe");
    xmpp_send(conn, subscribe);
    xmpp_stanza_release(subscribe);
  } else if (status == XMPP_CONN_DISCONNECT) {
    werr("Connection error: status XMPP_CONN_DISCONNECT");

    is_user_online = false;

    /* Signal disconnect event to files module */
    int rc;

    rc = pthread_mutex_lock(&mutex);
    wsyserr(rc != 0, "pthread_mutex_lock");

    signal_attr = true;
    signal_list = true;
    signal_read = true;
    signal_fail = true;
    rc = pthread_cond_signal(&cond);
    wsyserr(rc != 0, "pthread_cond_signal");

    rc = pthread_mutex_unlock(&mutex);
    wsyserr(rc != 0, "pthread_mutex_unlock");

    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  } else if (status == XMPP_CONN_FAIL) {
    werr("Connection error: status XMPP_CONN_FAIL");

    is_user_online = false;

    /* Signal disconnect event to files module */
    int rc;

    rc = pthread_mutex_lock(&mutex);
    wsyserr(rc != 0, "pthread_mutex_lock");

    signal_attr = true;
    signal_list = true;
    signal_read = true;
    signal_fail = true;
    rc = pthread_cond_signal(&cond);
    wsyserr(rc != 0, "pthread_cond_signal");

    rc = pthread_mutex_unlock(&mutex);
    wsyserr(rc != 0, "pthread_mutex_unlock");

    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  }

  wlog("Return from wconn_handler(...)");
}

/* Ping handler */
int wping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("w_ping_handler(...)");

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *pong = xmpp_stanza_new(ctx); /* pong stanza */
  xmpp_stanza_set_name(pong, "iq");
  xmpp_stanza_set_attribute(pong, "to", xmpp_stanza_get_attribute(stanza, "from"));
  xmpp_stanza_set_id(pong, xmpp_stanza_get_id(stanza));
  xmpp_stanza_set_type(pong, "result");
  xmpp_send(conn, pong);
  xmpp_stanza_release(pong);

  wlog("Return TRUE from w_ping_handler(...)");
  return TRUE;
}

/* Wyliodrin handler */
int wyliodrin_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("wyliodrin_handler(...)");

  /* Get stanza type */
  char *type = xmpp_stanza_get_type(stanza); /* Stanza type */

  /* Check for error type */
  int error = 0; /* error in type */
  if(type != NULL && strncasecmp(type, "error", 5) == 0) {
    error = 1;
  }
  
  /* Get every function from stanza and execute it */
  char *ns; /* namespace */
  char *name; /* name */
  tag_function *function; /* tag function */
  xmpp_stanza_t *tag = xmpp_stanza_get_children(stanza); /* Stanza children */
  while(tag != NULL) {
    ns = xmpp_stanza_get_ns(tag);
    if(ns != NULL && strncasecmp(ns, WNS, strlen(WNS)) == 0) {
      name = xmpp_stanza_get_name(tag);
      function = hashmap_get(tags, name);
      if(function != NULL && *function != NULL) { 
        werr("Function available");
        (*function)(xmpp_stanza_get_attribute(stanza, "from"), 
          xmpp_stanza_get_attribute(stanza, "to"), error, tag, conn, userdata);
      } else {
        werr("Function not available");
      }
    }
    tag = xmpp_stanza_get_next(tag);
  }

  wlog("Return TRUE from wyliodrin_handler(...)");
  return TRUE;
}

int wpresence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("wpresence_handler(...)");

  /* Presence type */
  char *type = xmpp_stanza_get_type(stanza); /* Stanza type */
  if (type == NULL) {
    xmpp_stanza_t *children = xmpp_stanza_get_child_by_name(stanza, "status");

    /* User is online */
    if (children != NULL) {
      is_user_online = true;

      /* Init shells module */
#     ifdef SHELLS
        init_shells();
#     endif

      /* Init files module */
#     ifdef SHELLS
        init_files();
#     endif
    }

    return TRUE;
  }

  if (strcmp(type, "subscribe") == 0) {
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

    xmpp_stanza_t *subscribed = xmpp_stanza_new(ctx); /* Subscribe stanza */
    xmpp_stanza_set_name(subscribed, "presence");
    xmpp_stanza_set_attribute(subscribed, "to", owner_str);
    xmpp_stanza_set_type(subscribed, "subscribed");
    xmpp_send(conn, subscribed);
    xmpp_stanza_release(subscribed);
  }

  else if (strcmp(type, "unavailable") == 0) {
    is_user_online = false;
  }

  wlog("Return TRUE from wpresence_handler(...)");
  return TRUE;
}
