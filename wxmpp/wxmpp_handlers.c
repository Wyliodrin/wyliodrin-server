/**************************************************************************************************
 * XMPP handlers: connection, ping
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */

#include "../winternals/winternals.h"

/* Ping handler */
int wxmpp_ping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
	wlog("wxmpp_ping_handler(...)");

	xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

	xmpp_stanza_t *pong = xmpp_stanza_new(ctx); /* pong response */
	xmpp_stanza_set_name(pong, "iq");
	xmpp_stanza_set_attribute(pong, "to", xmpp_stanza_get_attribute(stanza, "from"));
	xmpp_stanza_set_id(pong, xmpp_stanza_get_id(stanza));
	xmpp_stanza_set_type(pong, "result");
	xmpp_send(conn, pong);
	xmpp_stanza_release(pong);

	wlog("Returning 1 from wxmpp_ping_handler(...)");
	return 1;
}

/* Wyliodrin connection handler */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata) {
  wlog("wconn_handler(...)");

  if (status == XMPP_CONN_CONNECT) {
    wlog("Connection success");

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
    
    /* Add ping handler */
    xmpp_handler_add(conn, wxmpp_ping_handler, "urn:xmpp:ping", "iq", "get", ctx);
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
