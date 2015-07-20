/**************************************************************************************************
 * MSGPACK decoder
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#include <string.h>

#include "../winternals/winternals.h" /* logs and errs   */
#include "../wxmpp/wxmpp.h"           /* nameserver      */
#include "../base64/base64.h"         /* base64 encoding */

#include <stdbool.h>                  /* true and false  */
#include "../cmp/cmp.h"               /* msgpack         */

#include "../libds/ds.h"              /* hashmaps        */



#define SBUFSIZE 128



void upload2(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata, hashmap_p hm);



void msgpack_decoder(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
  /* Get msgpack data from d attribute */
  char *d_attr = xmpp_stanza_get_attribute(stanza, "d");
  if (d_attr == NULL) {
    werr("No attribute named d in w stanza");
    return;
  }

  /* Decode msgpack data */
  int dec_size = strlen(d_attr) * 3 / 4 + 1;
  uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
  int rc_int = base64_decode(decoded, d_attr, dec_size);
  if (rc_int == -1) {
    werr("Failed to decode msgpack data from d attribute");
    return;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  uint32_t map_size, string_size;
  char storage[STORAGESIZE];
  char sbuf[SBUFSIZE];
  char *k;
  memcpy(storage, decoded, rc_int);
  cmp_init(&cmp, storage);

  /* Read dictionary size */
  if (!cmp_read_map(&cmp, &map_size)) {
    werr("cmp_read_map: %s", cmp_strerror(&cmp));
    return;
  }

  /* Init hashmap */
  hashmap_p hm = create_hashmap();

  /* Read dictionary keys and values */
  int i;
  for (i = 0; i < map_size; i++) {
    /* Read key */
    string_size = SBUFSIZE;
    if (!cmp_read_str(&cmp, sbuf, &string_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      goto clean;
    }
    k = strdup(sbuf);

    /* Read value */
    if (k[0] == 's') { /* Read string */
      string_size = SBUFSIZE;
      if (!cmp_read_str(&cmp, sbuf, &string_size)) {
        werr("cmp_read_str: %s", cmp_strerror(&cmp));
        goto clean;
      }
      char *v_s = strdup(sbuf);
      hashmap_put(hm, k, v_s, string_size);
    } else if (k[0] == 'n') { /* Read number */
      int64_t *v_i = malloc(sizeof(int64_t));
      if (!cmp_read_integer(&cmp, v_i)) {
        werr("cmp_read_integer: %s", cmp_strerror(&cmp));
        goto clean;
      }
      hashmap_put(hm, k, v_i, sizeof(int64_t));
    } else if (k[0] == 'b') { /* Read binary */
      uint32_t v_b_size = BINSIZE;
      void *v_b = malloc(v_b_size);
      if (!cmp_read_bin(&cmp, v_b, &v_b_size)) {
        werr("cmp_read_bin: %s", cmp_strerror(&cmp));
        goto clean;
      }
    } else {
      werr("Unknown key: %s", k);
    }
  }

  upload2(from, to, error, stanza, conn, userdata, hm);

  clean:
    destroy_hashmap(hm);
}
