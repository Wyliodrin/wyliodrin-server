/**************************************************************************************************
 * Fuse module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifdef FILES

#include <strophe.h>

#define FUSE_USE_VERSION 30
#include <fuse.h>

#include <sys/types.h>

#include "files.h"

void files(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata) {

}

int wfiles_getattr(const char *path, struct stat *stbuf) {
  return 0;
}

int wfiles_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                   off_t offset, struct fuse_file_info *fi) {
  return 0;
}

#endif /* FILES */
