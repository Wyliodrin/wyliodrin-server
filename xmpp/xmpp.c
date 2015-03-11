/**************************************************************************************************
 * XMPP stuff
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */

#include "../internals/internals.h" /* logs and errs */
#include "xmpp.h" /* XMPP connection stuff */

/* Wyliodrin connection handler */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata) {
  wlog("wconn_handler(...)");

  if (status == XMPP_CONN_CONNECT) {
    wlog("Connection success");
    xmpp_disconnect(conn);
  } else {
    werr("Connection error");

    xmpp_ctx_t *ctx = (xmpp_ctx_t *)userdata;
    xmpp_stop(ctx);
  }

  wlog("Return from wconn_handler(...)");
}

int8_t wxmpp_connect(const char *jid, const char *pass) {
  wlog("wxmpp_connect(%s, %s)", jid, pass);

  /* Initialize the Strophe library */
  xmpp_initialize();

  /* Get Strophe logger, context and connection */
  xmpp_log_t *log  = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* Strophe logger */
  xmpp_ctx_t *ctx  = xmpp_ctx_new(NULL, log); /* Strophe context */
  if(ctx == NULL) {
    xmpp_shutdown();

    wlog("Return -1 due to NULL Strophe context");
    return -1;
  }
  xmpp_conn_t *conn = xmpp_conn_new(ctx); /* Strophe connection */
  if(conn == NULL) {
    xmpp_ctx_free(ctx);
    xmpp_shutdown();

    wlog("Return -2 due to NULL Strophe connection");
    return -2;
  }

  /* Setup authentication information */
  xmpp_conn_set_jid(conn, jid);
  xmpp_conn_set_pass(conn, pass);

  /* Initiate connection */
  if(xmpp_connect_client(conn, NULL, WXMPP_PORT, wconn_handler, ctx) < 0) {
    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);
    xmpp_shutdown();

    wlog("Return -3 due to connection error to XMPP server");
    return -3;
  }

  /* Enter the event loop */
  xmpp_run(ctx);

  /* Cleaning */
  xmpp_conn_release(conn);
  xmpp_ctx_free(ctx);
  xmpp_shutdown();

  wlog("Retun 0 on success");
  return 0;
}
