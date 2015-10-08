/**************************************************************************************************
 * Shells module API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifdef SHELLS

#ifndef _SHELLS_H
#define _SHELLS_H



/*** INCLUDES ************************************************************************************/

#include <strophe.h> /* xmpp library */

/*************************************************************************************************/



/*** API *****************************************************************************************/

/**
 * Initialize with NULL shells_vector
 */
void init_shells();

/**
 * Parse shells commands
*/
void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata);

/**
 * Start projects that died on board shutdown.
 */
void start_dead_projects();

/*************************************************************************************************/



#endif /* _SHELLS_H */

#endif /* SHELLS */
