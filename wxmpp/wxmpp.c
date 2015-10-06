/**************************************************************************************************
 * XMPP implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <unistd.h> /* usleep */

#include "../winternals/winternals.h" /* logs and errs */

#include "wxmpp.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define XMPP_PORT 5222 /* XMPP server port */

#define CONN_INTERVAL 1 /* Time in seconds between connection attempts */

/*************************************************************************************************/



/*** VARIABLES ***********************************************************************************/

xmpp_ctx_t *ctx = NULL;   /* XMPP context    */
xmpp_conn_t *conn = NULL; /* XMPP connection */

bool is_xmpp_connection_set = false;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern char *board;

/*************************************************************************************************/


/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

extern void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                         xmpp_stream_error_t * const stream_error, void * const userdata);

/*************************************************************************************************/


/*** API implementation **************************************************************************/

/* Connect to XMPP */
void xmpp_connect(const char *jid, const char *pass) {
  xmpp_initialize();

  #ifdef VERBOSE
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
  #else
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_WARN);
  #endif

  ctx = xmpp_ctx_new(NULL, log);
  werr2(ctx == NULL, xmpp_shutdown(); return, "Could not get XMPP context");

  conn = xmpp_conn_new(ctx); /* Strophe connection */
  werr2(conn == NULL, xmpp_ctx_free(ctx); xmpp_shutdown(); return,
        "Could not create XMPP connection");

  /* Setup authentication information */
  xmpp_conn_set_jid(conn, jid);
  xmpp_conn_set_pass(conn, pass);

  /* Initiate connection in loop */
  while (1) {
    int conn_rc = xmpp_connect_client(conn, NULL, XMPP_PORT, conn_handler, ctx);
    if (conn_rc < 0) {
      werr("Attempt to connect to XMPP server failed. Retrying...");
      sleep(CONN_INTERVAL);
    } else {
      break;
    }
  }

  /* Enter the event loop */
  xmpp_run(ctx);

  /* Event loop should run forever */
  werr("XMPP event loop completed. Retrying to connect...");

  /* Cleaning */
  is_xmpp_connection_set = false;
  xmpp_conn_release(conn);
  xmpp_ctx_free(ctx);
  xmpp_shutdown();
  ctx = NULL;
  conn = NULL;

  /* Retry to connect */
  if (strcmp(board, "server") != 0) {
    sleep(CONN_INTERVAL);
    return xmpp_connect(jid, pass);
  }
}

/*************************************************************************************************/
