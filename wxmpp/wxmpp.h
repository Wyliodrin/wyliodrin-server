/**************************************************************************************************
 * XMPP connection
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _WXMPP_H
#define _WXMPP_H

#define WNS "wyliodrin" /* Wyliodrin namespace */

#define TAGS_SIZE  100  /* Size of tags hashmap */
#define WXMPP_PORT 9091 /* Wyliodrin XMPP server port */

typedef void (*tag_function)(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
                             xmpp_conn_t *const conn, void *const userdata);

/**
 * Connect to Wyliodrin XMPP server
 *
 * PARAMETERS
 *    jid  - user
 *    pass - password
 *
 * RETURN
 *    -1 : NULL Strophe context
 *    -2 : NULL Strophe connection
 *    -3 : Connection error to XMPP server
 */
int8_t wxmpp_connect(const char *jid, const char *pass);

/**
 * Add tag function
 *
 * PARAMETERS:
 *    tag - tag name 
 *    f   - tag function
 */
void wadd_tag(char *tag, tag_function f);

#endif // _WXMPP_H
