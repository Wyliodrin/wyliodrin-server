/**************************************************************************************************
 * POWEROFF module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#include <stdlib.h> /* system */
#include <string.h> /* strcmp */

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"



extern const char *board_str; /* board name from wtalk.c   */
extern const char *sudo_str;  /* sudo command from wtalk.c */



void poweroff(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
  wlog("poweroff");

  if (strcmp(board_str, "server") != 0) {
    char cmd[64];
    sprintf(cmd, "%s poweroff", sudo_str);
    system(cmd);
  }

  exit(EXIT_SUCCESS);
}
