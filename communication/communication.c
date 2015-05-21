/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifdef COMMUNICATION

#include <stdio.h>
#include <strophe.h>

#include "../winternals/winternals.h" /* logs and errs */

void init_communication() {

}

void communication(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
	wlog("communication");
}

#endif /* COMMUNICATION */
