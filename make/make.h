/**************************************************************************************************
 * Make module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifndef _MAKE_H
#define _MAKE_H

#ifdef MAKE

void init_make();

void make(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
          xmpp_conn_t *const conn, void *const userdata);

#endif /* MAKE */

#endif /* _MAKE_H */
