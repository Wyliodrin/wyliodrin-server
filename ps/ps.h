/**************************************************************************************************
 * PS module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifdef PS

#ifndef _PS_H
#define _PS_H

/**
 * Parse ps commands
 */
void ps(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
        xmpp_conn_t *const conn, void *const userdata);

#endif /* _PS_H */

#endif /* PS */
