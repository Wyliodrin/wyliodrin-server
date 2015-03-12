/**************************************************************************************************
 * XMPP stuff
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

#endif // _XMPP_H
