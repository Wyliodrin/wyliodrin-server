/**************************************************************************************************
 * MessagePack decoding
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef USEMSGPACK



/*** INCLUDES ************************************************************************************/

#include <stdint.h> /* numbers handling */
#include <string.h> /* string handling  */

#include "../winternals/winternals.h" /* logs and errs    */
#include "../cmp/cmp.h"               /* msgpack handling */

#include "wmsgpack.h"

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define MSGPACK_KEY_SIZE     8
#define MSGPACK_MODULE_SIZE 32

/*************************************************************************************************/



/*** IMPLEMENTATIONS *****************************************************************************/

void wmsgpack(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
              xmpp_conn_t *const conn, void *const userdata) {
  /* Get the msgpack encoded data */
  char *text = xmpp_stanza_get_text_ptr(stanza);
  if (text == NULL) {
    werr("Could not get the msgpack encoded data");
    return;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  cmp_init(&cmp, text, strlen(text));

  /* Read msgpack map */
  uint32_t map_size;
  if (!cmp_read_map(&cmp, &map_size)) {
    werr("cmp_read_map: %s", cmp_strerror(&cmp));
    return;
  }

  /* Iterate over map in order to find the module */
  uint32_t i;
  char key[MSGPACK_KEY_SIZE];
  uint32_t str_size;
  for (i = 0; i < map_size; i++) {
    str_size = MSGPACK_KEY_SIZE;
    if (!cmp_read_str(&cmp, key, &str_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      return;
    }

    if (strcmp(key, "m") == 0) {
      /* Read module */
      char module[MSGPACK_MODULE_SIZE];
      str_size = MSGPACK_MODULE_SIZE;
      if (!cmp_read_str(&cmp, module, &str_size)) {
        werr("cmp_read_str: %s", cmp_strerror(&cmp));
        return;
      }

      /* Call module */


      return;
    }
  }
}

/*************************************************************************************************/



#endif USEMSGPACK
