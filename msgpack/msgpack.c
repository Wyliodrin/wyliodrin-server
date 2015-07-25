/**************************************************************************************************
 * MSGPACK decoder
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#ifdef USEMSGPACK



#include <string.h>                   /* memcpy, strlen  */

#include "../winternals/winternals.h" /* logs and errs   */
#include "../wxmpp/wxmpp.h"           /* nameserver      */
#include "../base64/base64.h"         /* base64 encoding */

#include <stdbool.h>                  /* true and false  */
#include "../cmp/cmp.h"               /* msgpack         */

#include "../libds/ds.h"              /* hashmaps        */



#define SBUFSIZE 128 /* Size of the string buffer used for cmp_read_str */



#ifdef UPLOAD
  void upload(const char *from, const char *to, xmpp_conn_t *const conn, void *const userdata,
    hashmap_p hm);
#endif

#ifdef SHELLS
  void shells(xmpp_conn_t *const conn, void *const userdata, hashmap_p hm);
#endif



void msgpack_decoder(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
  wlog("msgpack_decoder()");

  /* Get msgpack data from d attribute */
  char *d_attr = xmpp_stanza_get_attribute(stanza, "d");
  if (d_attr == NULL) {
    werr("No attribute named d in w stanza");
    return;
  }

  /* Decode msgpack data */
  int dec_size = strlen(d_attr) * 3 / 4 + 1;
  uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
  dec_size = base64_decode(decoded, d_attr, dec_size);
  if (dec_size == -1) {
    werr("Failed to decode msgpack data from d attribute");
    return;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  char storage[STORAGESIZE];
  char sbuf[SBUFSIZE];

  memcpy(storage, decoded, dec_size);
  cmp_init(&cmp, storage);

  /* Read dictionary size */
  uint32_t map_size;
  if (!cmp_read_map(&cmp, &map_size)) {
    werr("cmp_read_map error: %s", cmp_strerror(&cmp));
    return;
  }

  /* Init hashmap */
  hashmap_p hm = create_hashmap();

  /* Read dictionary keys and values */
  int i;
  char *k;
  uint32_t string_size;
  for (i = 0; i < map_size; i++) {
    /* Read key */
    string_size = SBUFSIZE;
    if (!cmp_read_str(&cmp, sbuf, &string_size)) {
      werr("cmp_read_str error: %s", cmp_strerror(&cmp));
      goto clean;
    }
    k = strdup(sbuf);

    /* Read value */
    if (k[0] == 's') { /* Read string */
      string_size = SBUFSIZE;
      if (!cmp_read_str(&cmp, sbuf, &string_size)) {
        werr("cmp_read_str error: %s", cmp_strerror(&cmp));
        goto clean;
      }
      char *v_s = strdup(sbuf);
      hashmap_put(hm, k, v_s, string_size);
    }

    else if (k[0] == 'n') { /* Read number */
      int64_t *v_i = malloc(sizeof(int64_t));
      if (!cmp_read_integer(&cmp, v_i)) {
        werr("cmp_read_integer error: %s", cmp_strerror(&cmp));
        goto clean;
      }
      hashmap_put(hm, k, v_i, sizeof(int64_t));
    }

    else if (k[0] == 'b') { /* Read binary */
      uint32_t v_b_size = BINSIZE;
      void *v_b = malloc(v_b_size);
      if (!cmp_read_bin(&cmp, v_b, &v_b_size)) {
        werr("cmp_read_bin: %s", cmp_strerror(&cmp));
        goto clean;
      }
      hashmap_put(hm, k, v_b, v_b_size);
    }

    else {
      werr("Unknown key: %s", k);
    }
  }

  char *module_name = (char *)hashmap_get(hm, "sm");
  if (module_name != NULL) {
    if (strcmp(module_name, "u") == 0) {
      #ifdef UPLOAD
        upload(from, to, conn, userdata, hm);
      #endif
    } else if (strcmp(module_name, "s") == 0) {
      #ifdef UPLOAD
        shells(conn, userdata, hm);
      #endif
    } else {
      werr("Unknown module: %s", module_name);
    }
  } else {
    werr("No sm key in msgpack data");
  }

  clean:
    destroy_hashmap(hm);
}

#endif /* USEMSGPACK */
