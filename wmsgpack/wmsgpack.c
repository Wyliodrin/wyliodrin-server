/**************************************************************************************************
 * MessagePack decoding
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef USEMSGPACK



/*** INCLUDES ************************************************************************************/

#include <errno.h>   /* errno            */
#include <stdbool.h> /* bool handling    */
#include <stdint.h>  /* numbers handling */
#include <string.h>  /* string handling  */

#include "../base64/base64.h"         /* base64 handling    */
#include "../cmp/cmp.h"               /* msgpack handling   */
#include "../libds/ds.h"              /* modules hashmap    */
#include "../winternals/winternals.h" /* logs and errs      */
#include "../wxmpp/wxmpp.h"           /* module_fct typedef */

#ifdef SHELLS
  #include "../shells/shells.h"
#endif /* SHELLS */

#ifdef FILES
  #include "../files/files.h"
#endif /* FILES */

#ifdef MAKE
  #include "../make/make.h"
#endif /* MAKE */

#ifdef COMMUNICATION
  #include "../communication/communication.h"
#endif /* COMMUNICATION */

#ifdef PS
  #include "../ps/ps.h"
#endif /* PS */

#ifdef DEBUG
  #include "../debug/debug.h"
#endif /* DEBUG */

#include "wmsgpack.h"

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define MSGPACK_BUF_SIZE   64

/*************************************************************************************************/



/*** TYPEDEFS ************************************************************************************/

typedef void (*module_handler)(const char *from, const char *to, hashmap_p h);

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern bool is_fuse_available;

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static hashmap_p modules = NULL;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static hashmap_p encoded_msgpack_map_to_hashmap (const char *encoded_msgpack_map);

/*************************************************************************************************/



/*** IMPLEMENTATIONS *****************************************************************************/

void build_modules_hashmap() {
  /* Sanity checks */
  if (modules != NULL) {
    werr("Sanity checks failed");
    return;
  }

  modules = create_hashmap();
  module_handler addr;

  #ifdef SHELLS
    addr = shells;
    hashmap_put(modules, "shells", &addr, sizeof(void *));
    init_shells();
    // start_dead_projects(conn, userdata);
  #endif
  #ifdef FILES
    addr = files;
    hashmap_put(modules, "files", &addr, sizeof(void *));
    if (is_fuse_available) {
      init_files();
    }
  #endif
  #ifdef MAKE
    addr = make;
    hashmap_put(modules, "make", &addr, sizeof(void *));
    init_make();
  #endif
  #ifdef COMMUNICATION
    addr = communication;
    hashmap_put(modules, "communication", &addr, sizeof(void *));
    init_communication();
  #endif
  #ifdef PS
    addr = ps;
    hashmap_put(modules, "ps", &addr, sizeof(void *));
  #endif
  #ifdef USEMSGPACK
    addr = wmsgpack;
    hashmap_put(modules, "w", &addr, sizeof(void *));
  #endif
}


char *_build_msgpack_map(int *size_addr, char *file, int line,
                        int num_params, ...) {
  /* Sanity checks */
  if (num_params % 2 == 1) {
    fprintf(stderr, "[BUILD_MSGPACK_MAP %s:%d] Uneven number of parameters\n", file, line);
    return NULL;
  }
  if (size_addr == NULL) {
    fprintf(stderr, "[BUILD_MSGPACK_MAP %s:%d] Second parameter is NULL\n", file, line);
  }

  va_list ap;

  /* Get size needed to store the map */
  int i;
  char *p;
  int params_size = 64;
  va_start(ap, num_params);
  for (i = 0; i < num_params; i++) {
    p = va_arg(ap, char *);
    if (p == NULL) {
      fprintf(stderr, "[BUILD_MSGPACK_MAP %s:%d] Argument %d is NULL\n", file, line, i);
      return NULL;
    }
    params_size += strlen(p);
  }
  va_end(ap);

  /* Allocate memory for msgpack map */
  char *ret_addr = malloc(params_size);
  if (ret_addr == NULL) {
    werr("malloc failed: %s", strerror(errno));
    return NULL;
  }

  /* Init msgpack */
  cmp_ctx_t cmp;
  cmp_init(&cmp, ret_addr, params_size);
  if (!cmp_write_map(&cmp, num_params / 2)) {
    werr("cmp_write_map error: %s", cmp_strerror(&cmp));
    free(ret_addr);
    return NULL;
  }

  /* Write msgpack map */
  va_start(ap, num_params);
  for (i = 0; i < num_params; i++) {
    p = va_arg(ap, char *);
    if (!cmp_write_str(&cmp, p, strlen(p))) {
      werr("cmp_write_map error: %s", cmp_strerror(&cmp));
      free(ret_addr);
      return NULL;
    }
  }
  va_end(ap);

  ret_addr[cmp.writer_offset] = 0;
  *size_addr = cmp.writer_offset;

  return ret_addr;
}


void wmsgpack(const char *from, const char *to, const char *enc_data) {
  /* Sanity checks */
  if (from == NULL ||
      to == NULL ||
      enc_data == NULL ||
      modules == NULL) {
    werr("Sanity checks failed");
    return;
  }

  /* Encoded data to hashmap */
  hashmap_p h = encoded_msgpack_map_to_hashmap(enc_data);

  /* Get module */
  char *module = (char *)hashmap_get(h, "m");
  if (module == NULL) {
    werr("There is no key named m in msgpack map");
    return;
  }

  /* Call module function */
  module_handler f = *((module_handler *)hashmap_get(modules, module));
  f(from, to, h);

  /* Clean */
  destroy_hashmap(h);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static hashmap_p encoded_msgpack_map_to_hashmap(const char *encoded_msgpack_map) {
  /* Sanity checks */
  if (encoded_msgpack_map == NULL) {
    werr("Sanity checks failed");
    return NULL;
  }

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

    hashmap_put(h, key, (void *)value, (size_t)(strlen(value)) + 1);
  }

  return h;
}

/*************************************************************************************************/



#endif /* USEMSGPACK */
