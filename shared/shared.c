/**************************************************************************************************
 * Shared data between the modules
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <stdbool.h>
#include <strophe.h>

#include "shared.h"

/*************************************************************************************************/



/*** SHARED VARIABLES ****************************************************************************/

xmpp_ctx_t *ctx;   /* XMPP context    */
xmpp_conn_t *conn; /* XMPP connection */

/* Values from wyliodrin.json */
const char *jid_str;
const char *owner_str;
const char *mount_file_str;
const char *build_file_str;
const char *board_str;
const char *sudo_str;
const char *shell_cmd;
bool privacy;
const bool is_fuse_available;

/*************************************************************************************************/
