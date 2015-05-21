/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#ifdef COMMUNICATION

void init_communication();

void communication(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata);

#endif /* COMMUNICATION */

#endif /* _COMMUNICATION_H */
