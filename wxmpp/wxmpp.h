/**************************************************************************************************
 * XMPP stuff: api
 *************************************************************************************************/

#ifndef _WXMPP_H
#define _WXMPP_H

#define WXMPP_PORT 9091 /* Wyliodrin XMPP server port */
#define WNS "wyliodrin" /* Wyliodrin namespace */

/**
 * Connect to Wyliodrin XMPP server
 *
 * PARAMETERS
 *    jid  -
 *    pass -
 *
 * RETURN
 *		-1 : NULL Strophe context
 *		-2 : NULL Strophe connection
 *    -3 : Connection error to XMPP server
 */
int8_t wxmpp_connect(const char *jid, const char *pass);

/**
 * Ping handler
 */
int wping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

/**
 * Connection handler
 */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata);

/**
 * Wyliodrin handler
 */
void wyliodrin_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                       xmpp_stream_error_t * const stream_error, void * const userdata)

#endif // _WXMPP_H
