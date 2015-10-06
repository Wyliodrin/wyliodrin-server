/**************************************************************************************************
 * XMPP handlers: connection, ping, presense, message
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#include <string.h>    /* strings handlings  */
#include <pthread.h>   /* mutex and cond     */
#include <Wyliodrin.h> /* version            */

#include "../winternals/winternals.h" /* logs and errs      */
#include "../libds/ds.h"              /* modules hashmap    */
#include "wxmpp.h"                    /* module_fct and WNS */
#include "wtalk_config.h"             /* version            */

#ifdef SHELLS
  #include "../shells/shells.h"
#endif

#ifdef FILES
  #include "../files/files.h"
#endif

#ifdef MAKE
  #include "../make/make.h"
#endif

#ifdef COMMUNICATION
  #include "../communication/communication.h"
#endif

#ifdef PS
  #include "../ps/ps.h"
#endif

#ifdef DEBUG
  #include "../debug/debug.h"
#endif



bool is_owner_online = false;

hashmap_p modules = NULL; /* modules hashmap */

/* Variables from files/files.c used for files synchronization */
extern pthread_mutex_t mutex;
extern pthread_cond_t  cond;
extern bool signal_attr;
extern bool signal_list;
extern bool signal_read;
extern bool signal_fail;

extern const char *owner; /* from wtalk.c */
extern bool is_fuse_available; /* from wtalk.c */
extern bool is_connected;


/* Module function signature */
typedef void (*module_fct)(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
                           xmpp_conn_t *const conn, void *const userdata);



/* Handlers */
int ping_handler     (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);
int presence_handler (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);
int message_handler  (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

/* Add the module_name and corresponding function in the modules hashmap */
void add_module(char *module_name, module_fct f);

static bool are_projects_initialized = false;



/* Connection handler */
void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                  xmpp_stream_error_t * const stream_error, void * const userdata) {
  wlog("conn_handler()");

  /* Connection success */
  if (status == XMPP_CONN_CONNECT) {
    wlog("XMPP connection success");
    is_connected = true;

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

    /* Create tags hashmap */
    if (!are_projects_initialized) {
      if (modules != NULL) {
        destroy_hashmap(modules);
      }
      modules = create_hashmap();

      /* Init modules */
      #ifdef SHELLS
        add_module("shells", shells);
        init_shells();
        start_dead_projects(conn, userdata);
      #endif
      #ifdef FILES
        add_module("files", files);
        if (is_fuse_available) {
          init_files();
        }
      #endif
      #ifdef MAKE
        add_module("make", make);
        init_make();
      #endif
      #ifdef COMMUNICATION
        add_module("communication", communication);
        init_communication();
      #endif
      #ifdef PS
        add_module("ps", ps);
      #endif

      are_projects_initialized = true;
    }

    /* Add ping handler */
    xmpp_handler_add(conn, ping_handler, "urn:xmpp:ping", "iq", "get", ctx);

    /* Add presence handler */
    xmpp_handler_add(conn, presence_handler, NULL, "presence", NULL, ctx);

    /* Add wyliodrin handler */
    xmpp_handler_add(conn, message_handler, WNS, "message", NULL, ctx);

    /* Send presence stanza:
       <presence><priority>50</priority></presence> */
    xmpp_stanza_t *presence_stz = xmpp_stanza_new(ctx); /* presence stanza */
    xmpp_stanza_set_name(presence_stz, "presence");
    xmpp_stanza_t *priority_stz = xmpp_stanza_new (ctx); /* priority stanza */
    xmpp_stanza_set_name(priority_stz, "priority");
    xmpp_stanza_add_child(presence_stz, priority_stz);
    xmpp_stanza_t *value_stz = xmpp_stanza_new(ctx); /* value stanza */
    xmpp_stanza_set_text(value_stz, "50");
    xmpp_stanza_add_child(priority_stz, value_stz);
    xmpp_send(conn, presence_stz);
    xmpp_stanza_release(value_stz);
    xmpp_stanza_release(priority_stz);
    xmpp_stanza_release(presence_stz);

    /* Send subscribe stanza
       <presence type="subscribe" to=<owner>/> */
    xmpp_stanza_t *subscribe_stz = xmpp_stanza_new(ctx); /* subscribe stanza */
    xmpp_stanza_set_name(subscribe_stz, "presence");
    xmpp_stanza_set_type(subscribe_stz, "subscribe");
    xmpp_stanza_set_attribute(subscribe_stz, "to", owner);
    xmpp_send(conn, subscribe_stz);
    xmpp_stanza_release(subscribe_stz);
  }

  /* Connection error */
  else {
    werr("XMPP connection error");

    is_owner_online = false;

    /* Signal disconnect event to files module */
    int rc_int;
    rc_int = pthread_mutex_lock(&mutex);
    wsyserr(rc_int != 0, "pthread_mutex_lock");
    signal_attr = true;
    signal_list = true;
    signal_read = true;
    signal_fail = true;
    rc_int = pthread_cond_signal(&cond);
    wsyserr(rc_int != 0, "pthread_cond_signal");
    rc_int = pthread_mutex_unlock(&mutex);
    wsyserr(rc_int != 0, "pthread_mutex_unlock");

    /* Stop the event loop */
    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  }
}


/**
 * Ping handler
 *
 * Ping stanza example:
 * <iq id=<id> to=<jid> type="get" from="wyliodrin.com">
 *   <ping xmlns="urn:xmpp:ping"/>
 * </iq>
 */
int ping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("w_ping_handler()");

  /* Sanity checks */
  char *from_attr = xmpp_stanza_get_attribute(stanza, "from"); /* from attribute */
  if (from_attr == NULL) {
    werr("Received ping without \"from\" attribute");
    return 0;
  }
  char *id_attr = xmpp_stanza_get_id(stanza); /* id attribute */
  if (id_attr == NULL) {
    werr("Received ping without \"id\" attribute");
    return 0;
  }

  /* Send pong stanza. Pong stanza:
   * <iq id=<id> type="result" to="wyliodrin.com"/>
   */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
  xmpp_stanza_t *pong_stz = xmpp_stanza_new(ctx); /* pong stanza */
  xmpp_stanza_set_name(pong_stz, "iq");
  xmpp_stanza_set_type(pong_stz, "result");
  xmpp_stanza_set_id(pong_stz, id_attr);
  xmpp_stanza_set_attribute(pong_stz, "to", from_attr);
  xmpp_send(conn, pong_stz);
  xmpp_stanza_release(pong_stz);

  return 1;
}


/* Presence handler */
int presence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("presence_handler()");

  char *type = xmpp_stanza_get_type(stanza); /* type of stanza */

  if (type == NULL) {
    /* User is online. Stanza example:
     * <presence to=<jid> from=<owner>>
     *    <priority>50</priority>
     *    <status>Happily echoing your &lt;message/&gt; stanzas</status>
     *  </presence>
     */
    if (xmpp_stanza_get_child_by_name(stanza, "status") != NULL) {
      wlog("Owner is online");

      is_owner_online = true;

      /* Send version */
      char wmajor[4];
      char wminor[4];
      char lwmajor[4];
      char lwminor[4];

      snprintf(wmajor,  4, "%d", WTALK_VERSION_MAJOR);
      snprintf(wminor,  4, "%d", WTALK_VERSION_MINOR);
      snprintf(lwmajor, 4, "%d", get_version_major());
      snprintf(lwminor, 4, "%d", get_version_minor());

      xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
      xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner);
      xmpp_stanza_t *version_stz = xmpp_stanza_new(ctx);
      xmpp_stanza_set_name(version_stz, "version");
      xmpp_stanza_set_ns(version_stz, WNS);
      xmpp_stanza_set_attribute(version_stz, "wmajor", wmajor);
      xmpp_stanza_set_attribute(version_stz, "wminor", wminor);
      xmpp_stanza_set_attribute(version_stz, "lwmajor", lwmajor);
      xmpp_stanza_set_attribute(version_stz, "lwminor", lwminor);
      xmpp_stanza_add_child(message_stz, version_stz);
      xmpp_send(conn, message_stz);
      xmpp_stanza_release(version_stz);
      xmpp_stanza_release(message_stz);
    }
  }

  /* This stanza is received once when the board is first added. Stanza example:
   * <presence to=<jid> type="subscribe" from=<owner>/>
   */
  else if (strcmp(type, "subscribe") == 0) {
    /* Send subscribed:
     * <presence type="subscribed" to=<owner>/>
     */
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
    xmpp_stanza_t *subscribed_stz = xmpp_stanza_new(ctx); /* subscribed stanza */
    xmpp_stanza_set_name(subscribed_stz, "presence");
    xmpp_stanza_set_attribute(subscribed_stz, "to", owner);
    xmpp_stanza_set_type(subscribed_stz, "subscribed");
    xmpp_send(conn, subscribed_stz);
    xmpp_stanza_release(subscribed_stz);
  }

  /* Owner becomes unavailable. Stanza example:
   *  <presence to=<jid> type="unavailable" from=<owner>/>
   */
  else if (strcmp(type, "unavailable") == 0) {
    wlog("Owner is unavailable");
    is_owner_online = false;
  }

  return 1;
}


/**
 * Message handler
 *
 * Example:
 * <message to=<jid> from=<owner>>
 *    <shells height="21" gadgetid=<jid> xmlns="wyliodrin" action="open" request=<request> width="90"/>
 * </message>
 */
int message_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("message_handler()");

  /* Sanity checks */
  char *from_attr = xmpp_stanza_get_attribute(stanza, "from");
  if (from_attr == NULL) {
    werr("Got message without \"from\" attribute");
    return 1;
  }
  char *to_attr = xmpp_stanza_get_attribute(stanza, "to");
  if (to_attr == NULL) {
    werr("Got message without \"to\" attribute");
    return 1;
  }

  /* Check for error type */
  char *type = xmpp_stanza_get_type(stanza); /* Stanza type */
  int error = 0; /* 1 if type of stanza is error, 0 otherwise */
  if(type != NULL && strncasecmp(type, "error", 5) == 0) {
    error = 1;
  }

  /* Get every module function from stanza and execute it */
  char *ns;      /* namespace       */
  char *name;    /* name            */
  module_fct *f; /* module function */
  xmpp_stanza_t *child_stz = xmpp_stanza_get_children(stanza); /* child of message */
  while(child_stz != NULL) {
    ns = xmpp_stanza_get_ns(child_stz);
    if(ns != NULL && strcmp(ns, WNS) == 0) {
      name = xmpp_stanza_get_name(child_stz);
      f = hashmap_get(modules, name);
      if(f != NULL && *f != NULL) {
        wlog("function available");
        (*f)(from_attr, to_attr, error, child_stz, conn, userdata);
      } else {
        werr("Module %s is not available", name);
      }
    }
    child_stz = xmpp_stanza_get_next(child_stz);
  }

  return 1;
}


/* Add the module_name and corresponding function in the modules hashmap */
void add_module(char *module_name, module_fct f) {
  hashmap_put(modules, module_name, &f, sizeof(void *));
}