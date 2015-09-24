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

#define MSGPACK_BUF_SIZE   64

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern hashmap_p modules; /* from wxmpp_handlers.c */

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static hashmap_p encoded_msgpack_map_to_hashmap(const char *encoded_msgpack_map);

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

  hashmap_p h = encoded_msgpack_map_to_hashmap(enc_data);
  char *module = (char *)hashmap_get(h, "m");
  if (module == NULL) {
    werr("There is no key named m in msgpack map");
    return;
  }

  module_fct f = *((module_fct *)hashmap_get(modules, module));
  f(from, to, error, stanza, conn, userdata);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static hashmap_p encoded_msgpack_map_to_hashmap(const char *encoded_msgpack_map) {
  /* Decode the encoded data */
  int msgpack_map_size = strlen(encoded_msgpack_map) * 3 / 4 + 1;
  char *msgpack_map = calloc(msgpack_map_size, sizeof(char *));
  if (msgpack_map == NULL) {
    werr("calloc: %s", strerror(errno));
    return NULL;
  }
  if (base64_decode((uint8_t *)msgpack_map, encoded_msgpack_map, msgpack_map_size) == -1) {
    werr("Invalid encoded msgpack map");
    return NULL;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  cmp_init(&cmp, msgpack_map, strlen(msgpack_map));

  /* Read msgpack map */
  uint32_t map_size;
  if (!cmp_read_map(&cmp, &map_size)) {
    werr("cmp_read_map: %s", cmp_strerror(&cmp));
    return NULL;
  }

  uint32_t i;
  uint32_t str_size;
  char key[MSGPACK_BUF_SIZE];
  char value[MSGPACK_BUF_SIZE];
  hashmap_p h = create_hashmap();
  for (i = 0; i < map_size; i++) {
    /* Get key */
    str_size = MSGPACK_BUF_SIZE;
    if (!cmp_read_str(&cmp, key, &str_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      destroy_hashmap(h);
      return NULL;
    }

    /* Get value */
    str_size = MSGPACK_BUF_SIZE;
    if (!cmp_read_str(&cmp, value, &str_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      destroy_hashmap(h);
      return NULL;
    }

    hashmap_put(h, key, (void *)value, (size_t)(strlen(value)));
  }

  return h;
}

/*************************************************************************************************/



#endif /* USEMSGPACK */
