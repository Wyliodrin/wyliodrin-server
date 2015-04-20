/**************************************************************************************************
 * Files module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _FILES_H
#define _FILES_H

#ifdef FILES

void init_files();

void files(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata);

#endif /* FILES */

#endif /* _FILES_H */
