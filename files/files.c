/**************************************************************************************************
 * Files module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifdef FILES

#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <strophe.h>

#include "files.h"

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

void files(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata) {

}

int wfiles_getattr(const char *path, struct stat *stbuf)
{
  int res = 0;
  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else if (strcmp(path, hello_path) == 0) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(hello_str);
  } else
    res = -ENOENT;
  return res;
}

int wfiles_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
       off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;
  if (strcmp(path, "/") != 0)
    return -ENOENT;
  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);
  filler(buf, hello_path + 1, NULL, 0);
  return 0;
}

int wfiles_open(const char *path, struct fuse_file_info *fi)
{
  if (strcmp(path, hello_path) != 0)
    return -ENOENT;
  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;
  return 0;
}

int wfiles_read(const char *path, char *buf, size_t size, off_t offset,
       struct fuse_file_info *fi)
{
  size_t len;
  (void) fi;
  if(strcmp(path, hello_path) != 0) {
    return -ENOENT;
  }
  len = strlen(hello_str);

  if (offset < len) {
    if (offset + size > len) {
      size = len - offset;
    }
    memcpy(buf, hello_str + offset, size);
  } else {
    size = 0;
  }

  return size;
}

#endif /* FILES */
