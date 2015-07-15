/**************************************************************************************************
 * Upload files from board to cloud.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#ifdef UPLOAD

#include <stdio.h>  /* fprintf */
#include <string.h> /* memcpy  */

/* file operations */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "../winternals/winternals.h" /* logs and errs   */
#include "../wxmpp/wxmpp.h"           /* nameserver      */
#include "../base64/base64.h"         /* base64 encoding */

#include <stdbool.h>                  /* true and false  */
#include "../cmp/cmp.h"               /* msgpack         */



#define STORAGESIZE 1024 /* msgpack buffer size */
#define PATHSIZE 128     /* msgpack size */



typedef enum {
  ATTRIBUTES = 0,
  LIST       = 1,
  READ       = 2
} action_code_t;

typedef enum {
  DIRECTORY = 0,
  REGULAR   = 1,
  OTHER     = 2
} filetype_t;



static unsigned int reader_offset = 0;
static unsigned int writer_offset = 0;

static bool string_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
  if (reader_offset + limit > STORAGESIZE) {
    fprintf(stderr, "No more space available in string_reader\n");
    return false;
  }

  memcpy(data, ctx->buf + reader_offset, limit);
  reader_offset += limit;

  return true;
}

static size_t string_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
  if (writer_offset + count > STORAGESIZE) {
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

    /* Decode msgpack_text */
    int dec_size = strlen(msgpack_text) * 3 / 4 + 1;
    uint8_t *decoded = calloc(dec_size, sizeof(uint8_t));
    int rc_int = base64_decode(decoded, msgpack_text, dec_size);
    xmpp_free((xmpp_ctx_t *)userdata, msgpack_text);
    if (rc_int == -1) {
      werr("Failed to decode msgpack text");
      return;
    }

    /* Init msgpack */
    cmp_ctx_t cmp;
    char storage[STORAGESIZE];
    memcpy(storage, decoded, rc_int);
    cmp_init(&cmp, storage, string_reader, string_writer);
    reader_offset = 0;

    /* Read action code and path */
    uint32_t array_size;
    if (!cmp_read_array(&cmp, &array_size)) {
      werr("cmp_read_array: %s", cmp_strerror(&cmp));
      return;
    }
    int16_t action_code;
    if (!cmp_read_short(&cmp, &action_code)) {
      werr("cmp_read_short error: %s", cmp_strerror(&cmp));
      return;
    }
    char path[PATHSIZE];
    uint32_t path_size = PATHSIZE;
    if (!cmp_read_str(&cmp, path, &path_size)) {
      werr("cmp_read_str: %s", cmp_strerror(&cmp));
      return;
    }

    /* Send response */
    writer_offset = 0;

    /* Attributes response */
    if ((action_code_t) action_code == ATTRIBUTES) {
      /* Get file stat */
      struct stat file_stat;
      int stat_rc = stat(path, &file_stat);

      /* Set number of elements of the response array */
      int num_elem;
      if (stat_rc == -1) {
        num_elem = 2;
      } else {
        num_elem = 4;
      }

      /* Write response */
      if (!cmp_write_array(&cmp, num_elem)) {
        werr("cmp_write_array: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_integer(&cmp, ATTRIBUTES)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_str(&cmp, path, path_size)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (num_elem == 4) {
        filetype_t filetype;
        if (S_ISDIR(file_stat.st_mode)) {
          filetype = DIRECTORY;
        } else if (S_ISREG(file_stat.st_mode)) {
          filetype = REGULAR;
        } else {
          filetype = OTHER;
        }

        if (!cmp_write_integer(&cmp, filetype)) {
          werr("cmp_write_integer: %s", cmp_strerror(&cmp));
          return;
        }

        if (!cmp_write_integer(&cmp, (uint64_t)(file_stat.st_size))) {
          werr("cmp_write_integer: %s", cmp_strerror(&cmp));
          return;
        }
      }
    }

    /* List response */
    else if ((action_code_t) action_code == LIST) {
      DIR *d;
      struct dirent *dir;
      int num_elem;

      d = opendir(path);
      if (d == NULL) {
        num_elem = 2;
      } else {
        num_elem = 2;
        while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, ".")  == 0 ||
              strcmp(dir->d_name, "..") == 0 ) {
            continue;
          }
          num_elem++;
        }
        closedir(d);
      }

      /* Write response */
      if (!cmp_write_array(&cmp, num_elem)) {
        werr("cmp_write_array: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_integer(&cmp, LIST)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_str(&cmp, path, path_size)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (num_elem > 2) {
        d = opendir(path);
        while ((dir = readdir(d)) != NULL) {
          if (strcmp(dir->d_name, ".")  == 0 ||
              strcmp(dir->d_name, "..") == 0 ) {
            continue;
          }
          if (!cmp_write_str(&cmp, dir->d_name, strlen(dir->d_name))) {
            werr("cmp_write_short: %s", cmp_strerror(&cmp));
            return;
          }
        }
        closedir(d);
      }
    }

    /* Read response */
    else if ((action_code_t) action_code == READ) {
      int num_elem;
      int fd = open(path, O_RDONLY);
      if (fd == -1) {
        num_elem = 2;
      } else {
        num_elem = 3;
      }

      /* Write response */
      if (!cmp_write_array(&cmp, num_elem)) {
        werr("cmp_write_array: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_integer(&cmp, READ)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (!cmp_write_str(&cmp, path, path_size)) {
        werr("cmp_write_short: %s", cmp_strerror(&cmp));
        return;
      }
      if (num_elem == 3) {
        /* Get file stat */
        struct stat file_stat;
        stat(path, &file_stat);
        char read_buffer[file_stat.st_size];
        read(fd, read_buffer, file_stat.st_size);
        close(fd);

        if (!cmp_write_str(&cmp, read_buffer, file_stat.st_size)) {
          werr("cmp_write_short: %s", cmp_strerror(&cmp));
          return;
        }
      }
    }

    /* Send back the stanza */
    char *encoded_data = malloc(BASE64_SIZE(writer_offset));
    encoded_data = base64_encode(encoded_data, BASE64_SIZE(writer_offset),
    (const uint8_t *)storage, writer_offset);

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
