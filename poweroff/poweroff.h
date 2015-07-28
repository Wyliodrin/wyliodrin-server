/**************************************************************************************************
 * POWEROFF module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#ifndef _POWEROFF_H
#define _POWEROFF_H



void poweroff(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata);



#endif /* POWEROFF */
