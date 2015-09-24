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
#include <errno.h>  /* errno            */

#include "../base64/base64.h"         /* base64 handling    */
#include "../cmp/cmp.h"               /* msgpack handling   */
#include "../libds/ds.h"              /* modules hashmap    */
#include "../winternals/winternals.h" /* logs and errs      */
#include "../wxmpp/wxmpp.h"           /* module_fct typedef */

#include "wmsgpack.h"

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define MSGPACK_KEY_SIZE     8
#define MSGPACK_MODULE_SIZE 32

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern hashmap_p modules; /* from wxmpp_handlers.c */

/*************************************************************************************************/



/*** IMPLEMENTATIONS *****************************************************************************/

void wmsgpack(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
              xmpp_conn_t *const conn, void *const userdata) {
  /* Get the msgpack encoded data */
  char *enc_data = xmpp_stanza_get_attribute(stanza, "d");
  if (enc_data == NULL) {
    werr("Could not get the msgpack encoded data");
    return;
  }

  /* Decode the encoded data */
  int data_size = strlen(enc_data) * 3 / 4 + 1;
  char *data = malloc(data_size);
  if (data == NULL) {
    werr("malloc: %s", strerror(errno));
    return;
  }
  data[data_size - 1] = 0;
  if (base64_decode((uint8_t *)data, enc_data, data_size) == -1) {
    werr("Invalid input fed for base64 decoding");
    return;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  cmp_init(&cmp, data, strlen(data));

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

      fprintf(stderr, "Call %s\n", module);

      /* Call module */
      // module_fct f = *((module_fct *)hashmap_get(modules, module));
      // f(from, to, error, stanza, conn, userdata);

      return;
    }
  }
}

/*************************************************************************************************/



#endif /* USEMSGPACK */
