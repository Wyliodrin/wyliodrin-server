/**************************************************************************************************
 * XMPP connection
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#include <unistd.h>                   /* usleep */

#include "../winternals/winternals.h" /* logs and errs */
#include "wxmpp.h"                    /* XMPP stuff    */



xmpp_ctx_t *ctx;   /* Strophe Context */
xmpp_conn_t *conn; /* Strophe Connection */



/* Connection handler from wxmpp/wxmpp_handlers.h */
extern void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                         xmpp_stream_error_t * const stream_error, void * const userdata);



/* Connect to XMPP */
void xmpp_connect(const char *jid, const char *pass) {
  wlog("wxmpp_connect(%s, %s)", jid, pass);

  xmpp_initialize();

  #ifdef LOG
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* Strophe logger */
  #else
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_WARN);  /* Strophe logger */
  #endif

  ctx  = xmpp_ctx_new(NULL, log); /* Strophe context */
  if(ctx == NULL) {
    xmpp_shutdown();

    wlog("Could not get Strophe context");
    return;
  }

  conn = xmpp_conn_new(ctx); /* Strophe connection */
  if(conn == NULL) {
    xmpp_ctx_free(ctx);
    xmpp_shutdown();

    wlog("Could not get Strophe connection");
    return;
  }

  /* Setup authentication information */
  xmpp_conn_set_jid(conn, jid);
  xmpp_conn_set_pass(conn, pass);

  /* Initiate connection in loop */
  int conn_rc;
  while (1) {
    conn_rc = xmpp_connect_client(conn, NULL, XMPP_PORT, conn_handler, ctx);
    if (conn_rc < 0) {
      usleep(1000000);
    } else {
      break;
    }
  }

  /* Enter the event loop */
  xmpp_run(ctx);

  /* Event loop should run forever */
  werr("XMPP event loop completed. Retrying to connect...");

  /* Cleaning */
  xmpp_conn_release(conn);
  xmpp_ctx_free(ctx);
  xmpp_shutdown();

  /* Retry to connect */
  return xmpp_connect(jid, pass);
}
