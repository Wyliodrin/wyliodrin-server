/**************************************************************************************************
 * XMPP stuff: api
 *************************************************************************************************/

#ifndef _WXMPP_H
#define _WXMPP_H

#define TAGS_SIZE  100  /* Size of tags hashmap */
#define WXMPP_PORT 9091 /* Wyliodrin XMPP server port */
#define WNS "wyliodrin" /* Wyliodrin namespace */

typedef void (*tag_function)(const char *from, const char *to, int error, xmpp_stanza_t *stanza);

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

#endif // _WXMPP_H
