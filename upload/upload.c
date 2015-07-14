/**************************************************************************************************
 * Upload files from board to cloud.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#ifdef UPLOAD

#include <stdio.h>  /* fprintf */
#include <string.h> /* memcpy  */

#include "../winternals/winternals.h" /* logs and errs   */
#include "../wxmpp/wxmpp.h"           /* nameserver      */
#include "../base64/base64.h"         /* base64 encoding */

#include <stdbool.h>                  /* true and false  */
#include "../cmp/cmp.h"               /* msgpack         */



#define PATHSIZE 128



static unsigned int storagesize = 0;
static unsigned int reader_offset = 0;
static unsigned int writer_offset = 0;



static bool string_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  if (reader_offset + limit > storagesize) {
    fprintf(stderr, "No more space available in string_reader\n");
    return false;
  }

  memcpy(data, ctx->buf + reader_offset, limit);
  reader_offset += limit;

  return true;
}

static size_t string_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  if (writer_offset + count > storagesize) {
    fprintf(stderr, "No more space available in string_writer\n");
    return 0;
  }

  memcpy(ctx->buf + writer_offset, data, count);
  writer_offset += count;

  return count;
}


/**
 *  <message to="<jid"> from="<owner>">
 *    <upload xmlns="wyliodrin">
 *       <msgpack>X</msgpack>
 *    </upload>
 *  </message>
 */
void upload(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
  xmpp_stanza_t *msgpack_child = xmpp_stanza_get_child_by_name(stanza, "msgpack");

  /* Process msgpack message */
  if (msgpack_child != NULL) {
    char *msgpack_text = xmpp_stanza_get_text(msgpack_child);
    if (msgpack_text == NULL) {
      werr("Upload with no text in msgpack child stanza");
      return;
    }

    int dec_size = strlen(msgpack_text) * 3 / 4 + 1;
    uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
    int rc_int = base64_decode(decoded, msgpack_text, dec_size);
    xmpp_free((xmpp_ctx_t *)userdata, msgpack_text);
    if (rc_int == -1) {
      werr("Failed to decode msgpack text");
      return;
    }

    cmp_ctx_t cmp;
    storagesize = rc_int;
    cmp_init(&cmp, decoded, string_reader, string_writer);
    reader_offset = 0;

    uint32_t array_size;
    if (!cmp_read_array(&cmp, &array_size)) {
      werr("cmp_read_array: %s", cmp_strerror(&cmp));
      return;
    }

    int16_t code;
    if (!cmp_read_short(&cmp, &code)) {
      werr("cmp_read_short error: %s", cmp_strerror(&cmp));
      return;
    }

    char path[PATHSIZE];
    uint32_t path_size = PATHSIZE;
    if (!cmp_read_str(&cmp, path, &path_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      return;
    }

    wlog("code = %d, path = %s", code, path);

    /* Send back the stanza */
    char *encoded_data = malloc(BASE64_SIZE(rc_int));
    encoded_data = base64_encode(encoded_data, BASE64_SIZE(rc_int),
    (const uint8_t *)decoded, rc_int);

    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
    xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message_stz, "message");
    xmpp_stanza_set_attribute(message_stz, "to", "wyliodrin_test@wyliodrin.org");
    xmpp_stanza_t *upload_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(upload_stz, "upload");
    xmpp_stanza_set_ns(upload_stz, "wyliodrin");
    xmpp_stanza_t *msgpack_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(msgpack_stz, "msgpack");
    xmpp_stanza_t *text_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text_stz, encoded_data);
    xmpp_stanza_add_child(msgpack_stz, text_stz);
    xmpp_stanza_add_child(upload_stz, msgpack_stz);
    xmpp_stanza_add_child(message_stz, upload_stz);
    xmpp_send(conn, message_stz);
    xmpp_stanza_release(text_stz);
    xmpp_stanza_release(msgpack_stz);
    xmpp_stanza_release(upload_stz);
    xmpp_stanza_release(message_stz);
  }

  /* Process normal message */
  else {
    werr("Upload processing without msgpack not implemented yet");
  }
}

#endif /* UPLOAD */
