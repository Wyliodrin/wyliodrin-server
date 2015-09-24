/**************************************************************************************************
 * MessagePack decoding
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef USEMSGPACK

#ifndef _USEMSGPACK_H
#define _USEMSGPACK_H



/*** INCLUDES ************************************************************************************/

#include <strophe.h> /* XMPP handling */



/*** DECLARATIONS ********************************************************************************/

/* Handler for the w stanza */
void wmsgpack(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
              xmpp_conn_t *const conn, void *const userdata);



#endif /* _USEMSGPACK_H */

#endif /* USEMSGPACK */
