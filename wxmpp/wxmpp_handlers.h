/**************************************************************************************************
 * XMPP handlers
 *************************************************************************************************/

#ifndef _WXMPP_HANDLERS_H
#define _WXMPP_HANDLERS_H

extern hashmap_p tags;

/**
 * Connection handler
 */
void wconn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                   xmpp_stream_error_t * const stream_error, void * const userdata);

/**
 * Ping handler
 */
int wping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

/**
 * Wyliodrin handler
 */
int wyliodrin_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

#endif // _WXMPP_HANDLERS_H