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

#include "../libds/ds.h"              /* hashmaps        */



#define PATHSIZE 128           /* msgpack size */



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



static void attributes_response(char *path, xmpp_conn_t *const conn, void *const userdata,
  hashmap_p hm)
{
  /* Init msgpack */
  cmp_ctx_t cmp;
  char storage[STORAGESIZE];
  cmp_init(&cmp, storage);

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

  if (!cmp_write_map(&cmp, num_elem)) {
    werr("cmp_write_map error: %s", cmp_strerror(&cmp));
    return;
  }

  if (!cmp_write_str(&cmp, "p", 1)) {
    werr("cmp_write_short: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_str(&cmp, path, strlen(path))) {
    werr("cmp_write_short: %s", cmp_strerror(&cmp));
    return;
  }

  if (!cmp_write_str(&cmp, "c", 1)) {
    werr("cmp_write_str: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_integer(&cmp, ATTRIBUTES)) {
    werr("cmp_write_integer: %s", cmp_strerror(&cmp));
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

    if (!cmp_write_str(&cmp, "t", 1)) {
      werr("cmp_write_str: %s", cmp_strerror(&cmp));
      return;
    }
    if (!cmp_write_integer(&cmp, filetype)) {
      werr("cmp_write_integer: %s", cmp_strerror(&cmp));
      return;
    }

    if (!cmp_write_str(&cmp, "s", 1)) {
      werr("cmp_write_str: %s", cmp_strerror(&cmp));
      return;
    }
    if (!cmp_write_integer(&cmp, (uint64_t)(file_stat.st_size))) {
      werr("cmp_write_integer: %s", cmp_strerror(&cmp));
      return;
    }
  }

  /* Send back the stanza */
  char *encoded_data = malloc(BASE64_SIZE(cmp.writer_offset));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(cmp.writer_offset),
  (const uint8_t *)storage, cmp.writer_offset);

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
  xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", "wyliodrin_test@wyliodrin.org");
  xmpp_stanza_t *upload_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(upload_stz, "w");
  xmpp_stanza_set_ns(upload_stz, "wyliodrin");
  xmpp_stanza_set_attribute(upload_stz, "d", encoded_data);
  xmpp_stanza_add_child(message_stz, upload_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(upload_stz);
  xmpp_stanza_release(message_stz);

  free(encoded_data);
}



static void list_response(char *path, xmpp_conn_t *const conn, void *const userdata)
{
  /* Init msgpack */
  cmp_ctx_t cmp;
  char storage[STORAGESIZE];
  cmp_init(&cmp, storage);

  DIR *d;
  struct dirent *dir;
  int num_files = 0;

  d = opendir(path);
  if (d != NULL) {
    while ((dir = readdir(d)) != NULL) {
      num_files++;
    }
    closedir(d);
  }

  /* Write response */
  if (!cmp_write_map(&cmp, num_files == 0 ? 2 : 3)) {
    werr("cmp_write_map error: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_str(&cmp, "c", 1)) {
    werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_integer(&cmp, LIST)) {
    werr("cmp_write_integer error: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_str(&cmp, "p", 1)) {
    werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_str(&cmp, path, strlen(path))) {
    werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    return;
  }
  if (num_files > 0) {
    if (!cmp_write_str(&cmp, "l", 1)) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
      return;
    }

    if (!cmp_write_array(&cmp, num_files)) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
      return;
    }

    d = opendir(path);
    while ((dir = readdir(d)) != NULL) {
      if (!cmp_write_str(&cmp, dir->d_name, strlen(dir->d_name))) {
        werr("cmp_write_str error: %s", cmp_strerror(&cmp));
        return;
      }
    }
    closedir(d);
  }

  /* Send back the stanza */
  char *encoded_data = malloc(BASE64_SIZE(cmp.writer_offset));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(cmp.writer_offset),
  (const uint8_t *)storage, cmp.writer_offset);

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
  xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", "wyliodrin_test@wyliodrin.org");
  xmpp_stanza_t *upload_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(upload_stz, "w");
  xmpp_stanza_set_ns(upload_stz, "wyliodrin");
  xmpp_stanza_set_attribute(upload_stz, "d", encoded_data);
  xmpp_stanza_add_child(message_stz, upload_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(upload_stz);
  xmpp_stanza_release(message_stz);

  free(encoded_data);
}



static void read_response(int fd, int offset, size_t size, char *path,
  xmpp_conn_t *const conn, void *const userdata, hashmap_p hm)
{
  int num_elem;
  char done = 0;
  int num_bytes_read;

  if (fd == -1) {
    num_elem = 2;
  } else {
    num_elem = 5;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  char storage[STORAGESIZE];
  cmp_init(&cmp, storage);

  /* Write response */
  if (!cmp_write_map(&cmp, num_elem)) {
    werr("cmp_write_map: %s", cmp_strerror(&cmp));
    return;
  }

  if (!cmp_write_str(&cmp, "c", 1)) {
    werr("cmp_write_str: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_integer(&cmp, READ)) {
    werr("cmp_write_integer: %s", cmp_strerror(&cmp));
    return;
  }

  if (!cmp_write_str(&cmp, "p", 1)) {
    werr("cmp_write_str: %s", cmp_strerror(&cmp));
    return;
  }
  if (!cmp_write_str(&cmp, path, strlen(path))) {
    werr("cmp_write_str: %s", cmp_strerror(&cmp));
    return;
  }

  if (num_elem == 5) {
    /* Get file stat */
    char read_buffer[BINSIZE];
    num_bytes_read = read(fd, read_buffer, BINSIZE);

    if (!cmp_write_str(&cmp, "d", 1)) {
      werr("cmp_write_str: %s", cmp_strerror(&cmp));
      return;
    }
    if (!cmp_write_bin(&cmp, (const void *)read_buffer, num_bytes_read)) {
      werr("cmp_write_bin: %s", cmp_strerror(&cmp));
      return;
    }

    if (!cmp_write_str(&cmp, "o", 1)) {
      werr("cmp_write_str: %s", cmp_strerror(&cmp));
      return;
    }
    if (!cmp_write_integer(&cmp, offset)) {
      werr("cmp_write_integer: %s", cmp_strerror(&cmp));
      return;
    }

    if (offset + num_bytes_read == size) {
      done = 1;
    }

    if (!cmp_write_str(&cmp, "e", 1)) {
      werr("cmp_write_str: %s", cmp_strerror(&cmp));
      return;
    }
    if (!cmp_write_integer(&cmp, done)) {
      werr("cmp_write_short: %s", cmp_strerror(&cmp));
      return;
    }
  }

  /* Send back the stanza */
  char *encoded_data = malloc(BASE64_SIZE(cmp.writer_offset));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(cmp.writer_offset),
  (const uint8_t *)storage, cmp.writer_offset);

  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;
  xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(message_stz, "message");
  xmpp_stanza_set_attribute(message_stz, "to", "wyliodrin_test@wyliodrin.org");
  xmpp_stanza_t *upload_stz = xmpp_stanza_new(ctx);
  xmpp_stanza_set_name(upload_stz, "w");
  xmpp_stanza_set_ns(upload_stz, "wyliodrin");
  xmpp_stanza_set_attribute(upload_stz, "d", encoded_data);
  xmpp_stanza_add_child(message_stz, upload_stz);
  xmpp_send(conn, message_stz);
  xmpp_stanza_release(upload_stz);
  xmpp_stanza_release(message_stz);

  free(encoded_data);

  if (num_elem == 5 && done == 0) {
    usleep(500000);
    return read_response(fd, offset + num_bytes_read, size, path, conn, userdata, hm);
  }
}



void upload(const char *from, const char *to, xmpp_conn_t *const conn, void *const userdata,
  hashmap_p hm)
{
  char *path = (char *)(hashmap_get(hm, "sp"));
  int64_t action_code = *((int64_t *)(hashmap_get(hm, "nc")));

  /* Init msgpack */
  cmp_ctx_t cmp;
  char storage[STORAGESIZE];
  cmp_init(&cmp, storage);

  /* Attributes response */
  if ((action_code_t) action_code == ATTRIBUTES) {
    attributes_response(path, conn, userdata, hm);
    return;
  }

  /* List response */
  else if ((action_code_t) action_code == LIST) {
    list_response(path, conn, userdata);
    return;
  }

  /* Read response */
  else if ((action_code_t) action_code == READ) {
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
      read_response(-1, -1, -1, path, conn, userdata, NULL);
    } else {
      struct stat file_stat;
      stat(path, &file_stat);
      read_response(fd, 0, file_stat.st_size, path, conn, userdata, hm);
      close(fd);
    }
    return;
  }
}

#endif /* UPLOAD */
