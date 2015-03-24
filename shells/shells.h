/**************************************************************************************************
 * Shells module
 *************************************************************************************************/

#ifndef _SHELLS_H
#define _SHELLS_H

#ifdef SHELLS

extern const char *owner_str; /* owner_str from init.c */

/**
 * Parse shells commands
 */
void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata);

/**
 * Open shell
 */
void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/**
 * Close shell
 */
void shells_close();

/**
 * Keys from shell
 */
void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

#endif /* SHELLS */

#endif /* _SHELLS_H */
