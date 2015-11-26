/**************************************************************************************************
 * Shells module API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/

#ifdef SHELLS

#ifndef _SHELLS_H
#define _SHELLS_H



/*** INCLUDES ************************************************************************************/

#include <strophe.h> /* xmpp library */

/*************************************************************************************************/



/*** API *****************************************************************************************/

/**
 * Shell initialization
 */
void init_shells();

/**
 * Parse shells commands
 */
void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata);

/*************************************************************************************************/



#endif /* _SHELLS_H */

#endif /* SHELLS */
