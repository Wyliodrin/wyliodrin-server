/**************************************************************************************************
 * XMPP handlers
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _WXMPP_HANDLERS_H
#define _WXMPP_HANDLERS_H

extern hashmap_p tags;        /* tags from wxmpp.c */
extern const char *owner_str; /* owner_str from init.c */

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

/**
 * Presence handler
 */
int wpresence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata);

#endif // _WXMPP_HANDLERS_H
