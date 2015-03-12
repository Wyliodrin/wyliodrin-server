/**************************************************************************************************
 * XMPP handlers: connection, ping
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */
#include <string.h>  /* strlen */

#include "../winternals/winternals.h"
#include "../libds/ds.h"
#include "wxmpp.h"
#include "wxmpp_handlers.h"

/* Wyliodrin connection handler */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata) {
  wlog("wconn_handler(...)");

  if (status == XMPP_CONN_CONNECT) {
    wlog("Connection success");

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
    
    /* Add ping handler */
    xmpp_handler_add(conn, wping_handler, "urn:xmpp:ping", "iq", "get", ctx);

    /* Add wyliodrin namespace */
    xmpp_handler_add(conn, wyliodrin_handler, WNS, "message", NULL, ctx);

    /* Send presence */
    xmpp_stanza_t *pres = xmpp_stanza_new(ctx); /* Presence stanza */
    xmpp_stanza_set_name(pres, "presence");
    xmpp_send(conn, pres);
    xmpp_stanza_release(pres);
  } else if (status == XMPP_CONN_DISCONNECT) {
    werr("Connection error: status XMPP_CONN_DISCONNECT");

    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  } else if (status == XMPP_CONN_FAIL) {
    werr("Connection error: status XMPP_CONN_FAIL");

    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  }

  wlog("Return from wconn_handler(...)");
}

/* Ping handler */
int wping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("wxmpp_ping_handler(...)");

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *pong = xmpp_stanza_new(ctx); /* pong response */
  xmpp_stanza_set_name(pong, "iq");
  xmpp_stanza_set_attribute(pong, "to", xmpp_stanza_get_attribute(stanza, "from"));
  xmpp_stanza_set_id(pong, xmpp_stanza_get_id(stanza));
  xmpp_stanza_set_type(pong, "result");
  xmpp_send(conn, pong);
  xmpp_stanza_release(pong);

  wlog("Returning TRUE from wxmpp_ping_handler(...)");
  return TRUE;
}

/* Wyliodrin handler */
int wyliodrin_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  wlog("wyliodrin_handler(...)");

  /* Get stanza type */
  char *type = xmpp_stanza_get_type(stanza); /* STanza type */

  /* Check for error type */
  int error = 0; /* error in type */
  if(type != NULL && strncasecmp(type, "error", 5) == 0) {
    error = 1;
  }
  
  /* Get every function and put the  */
  char *ns;   /* namespace */
  char *name; /* name */
  xmpp_stanza_t *tag = xmpp_stanza_get_children(stanza); /* Stanza children */
  while(tag != NULL) {
    ns = xmpp_stanza_get_ns(tag);
    if(ns!=NULL && strncasecmp(ns, WNS, strlen(WNS)) == 0) {
      name = xmpp_stanza_get_name(tag);
      tag_function *function = hashmap_get(tags, name);
      if(function != NULL && *function != NULL) { 
        (*function)(xmpp_stanza_get_attribute(stanza, "from"), xmpp_stanza_get_attribute(stanza, "to"), error, tag);
      }
      tag = xmpp_stanza_get_next(tag);
    }
  }

  wlog("Returning TRUE from wyliodrin_handler(...)");
  return TRUE;
}
