/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifndef _COMMUNICATION_H
#define _COMMUNICATION_H

#ifdef COMMUNICATION

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

#define URL_SIZE 128

#define WYLIODRIN_CHANNEL "wyliodrin"
#define PUB_CHANNEL       "communication_client"
#define SUB_CHANNEL       "communication_server:*"

#define HYPERVISOR_SUB_CHANNEL "whypsrv"
#define HYPERVISOR_PUB_CHANNEL "wsrvhyp"

void init_communication();

void communication(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata);

#endif /* COMMUNICATION */

#endif /* _COMMUNICATION_H */
