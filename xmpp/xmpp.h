/**************************************************************************************************
 * XMPP stuff: api
 *************************************************************************************************/

#ifndef _XMPP_H
#define _XMPP_H

#define WXMPP_PORT 9091 /* Wyliodrin XMPP server port */

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
int wxmpp_ping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

/**
 * Connection handler
 */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata);

#endif // _XMPP_H
