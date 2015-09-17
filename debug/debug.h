/**************************************************************************************************
 * Debug module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef DEBUG

#ifndef _DEBUG_H
#define _DEBUG_H



#include <strophe.h>



#define GDB_COMMANDS_CHANNEL "gdb_commands"
#define GDB_RESULTS_CHANNEL  "gdb_results"

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379



/**
 * Initialize debug
 */
void init_debug();

/**
 * Called when a debug stanza arrives
 */
void debug(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata);



#endif /* _DEBUG_H */

#endif /* DEBUG */
