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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <strophe.h>

#include "../winternals/winternals.h"
#include "../wxmpp/wxmpp.h"
#include "../base64/base64.h"
#include "files.h"

extern xmpp_ctx_t *ctx;   /* Context    */
extern xmpp_conn_t *conn; /* Connection */

extern const char *owner_str; /* owner_str from init.c */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;

unsigned char gotattr = 0;
unsigned char gotlist = 0;
unsigned char gotread = 0;

typedef enum {
  DIR, /* Directory    */
  REG  /* Regular file */
} TYPE;

typedef struct {
  unsigned int size;
  TYPE type;
  char valid;
} attr_t;

attr_t attributes = {0, DIR, -1};

typedef struct elem_t {
  TYPE type;
  char *filename;
  struct elem_t *next;
} elem_t;
elem_t *root = NULL;
elem_t *last = NULL;

char *read_data = NULL;

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

static int hello_getattr(const char *path, struct stat *stbuf) {
  wlog("hello_getattr path = %s\n", path);

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  if (rc != 0) {
    werr("pthread_mutex_lock");
    perror("pthread_mutex_lock");
  }

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);

  xmpp_stanza_t *files = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(files, "files");
  xmpp_stanza_set_ns(files, WNS);
  xmpp_stanza_set_attribute(files, "action", "attributes");
  xmpp_stanza_set_attribute(files, "path", path);

  xmpp_stanza_add_child(message, files);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  /* Wait until attributes is set */
  while (gotattr == 0) {
    pthread_cond_wait(&cond, &mutex);
  }

  /* Do your job */
  int res = 0;

  memset(stbuf, 0, sizeof(struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0444;
    stbuf->st_nlink = 2;
  }

  else {
    if (attributes.valid == 1) {
      if (attributes.type == DIR) {
        stbuf->st_mode = S_IFDIR | 0444;
        stbuf->st_nlink = 2;
        stbuf->st_size = attributes.size;
      } else if (attributes.type == REG) {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = attributes.size;
      } else {
        werr("Unknown type");
        res = -ENOENT;
      }
    } else {
      res = -ENOENT;
    }
  }

  /* Job done */

  gotattr = 0;

  rc = pthread_mutex_unlock(&mutex);
  if (rc != 0) {
    werr("pthread_unmutex_lock");
    perror("pthread_unmutex_lock");
  }

  return res;
}

static int hello_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
  wlog("hello_readdir path = %s\n", path);

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  if (rc != 0) {
    werr("pthread_mutex_lock");
    perror("pthread_mutex_lock");
  }

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);

  xmpp_stanza_t *files = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(files, "files");
  xmpp_stanza_set_ns(files, WNS);
  xmpp_stanza_set_attribute(files, "action", "list");
  xmpp_stanza_set_attribute(files, "path", path);

  xmpp_stanza_add_child(message, files);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  /* Wait until attributes is set */
  while (gotlist == 0) {
    pthread_cond_wait(&cond, &mutex);
  }

  /* Do your job */

  (void) offset;
  (void) fi;

  filler(buf, ".", NULL, 0);
  filler(buf, "..", NULL, 0);

  if (root == NULL) {
    werr("root is NULL");
  } else {
    elem_t *aux = root;
    while (aux != NULL) {
      filler(buf, aux->filename, NULL, 0);
      aux = aux->next;
    }
    root = NULL;
  }

  /* Job done */

  gotlist = 0;

  rc = pthread_mutex_unlock(&mutex);
  if (rc != 0) {
    werr("pthread_unmutex_lock");
    perror("pthread_unmutex_lock");
  }

  return 0;
}

static int hello_open(const char *path, struct fuse_file_info *fi) {
  wlog("hello_open path = %s\n", path);

  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  return 0;
}

static int hello_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
  wlog("hello_read path = %s\n", path);

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  if (rc != 0) {
    werr("pthread_mutex_lock");
    perror("pthread_mutex_lock");
  }

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);

  xmpp_stanza_t *files = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(files, "files");
  xmpp_stanza_set_ns(files, WNS);
  xmpp_stanza_set_attribute(files, "action", "read");
  xmpp_stanza_set_attribute(files, "path", path);

  xmpp_stanza_add_child(message, files);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  /* Wait until attributes is set */
  while (gotread == 0) {
    pthread_cond_wait(&cond, &mutex);
  }

  /* Do your job */

  size_t len;
  (void) fi;
  len = strlen(read_data);

  if (offset < len) {
    if (offset + size > len) {
      size = len - offset;
    }
    memcpy(buf, read_data + offset, size);
  } else {
    size = 0;
  }

  /* Job done */

  gotread = 0;

  rc = pthread_mutex_unlock(&mutex);
  if (rc != 0) {
    werr("pthread_unmutex_lock");
    perror("pthread_unmutex_lock");
  }

  return size;
}

static struct fuse_operations hello_oper = {
  .getattr        = hello_getattr,
  .readdir        = hello_readdir,
  .open           = hello_open,
  .read           = hello_read,
};

void *init_files_thread(void *a) {
  char *argv[] = {"dummy", "-s", "-f", "mnt"};
  fuse_main(4, argv, &hello_oper, NULL);
  return NULL;
}

void init_files() {
  pthread_t ift; /* Init files thread */
  int rc = pthread_create(&ift, NULL, init_files_thread, NULL);
  if (rc < 0) {
    werr("SYSERR pthread_create");
    perror("pthread_create");
    return;
  }
}

void files(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata) {
  wlog("files()");

  int rc; /* Return code */

  /* Get action attribute */
  char *action_attr = xmpp_stanza_get_attribute(stanza, "action"); /* action attribute */
  if (action_attr == NULL) {
    werr("xmpp_stanza_get_attribute attribute = action");
  }

  /* attributes action */
  if (strncasecmp(action_attr, "attributes", 10) == 0) {
    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_lock");
      perror("pthread_mutex_lock");
    }

    /* Get error attribute */
    char *error_attr = xmpp_stanza_get_attribute(stanza, "error"); /* action attribute */
    if (error_attr == NULL) {
      werr("xmpp_stanza_get_attribute attribute = error");
      return;
    }

    if (strncasecmp(error_attr, "0", 1) != 0) {
      wlog("Error in attributes: %s", error_attr);
    } else {
      /* Get type attributes */
      char *type_attr = xmpp_stanza_get_attribute(stanza, "type");
      if (type_attr == NULL) {
        werr("xmpp_stanza_get_attribute attribute = type");
      }

      if (strncasecmp(type_attr, "directory", 9) == 0) {
        attributes.valid = 1;
        attributes.type = DIR;
        
        /* Get size attributes */
        char *size_attr = xmpp_stanza_get_attribute(stanza, "size");
        if (size_attr == NULL) {
          werr("xmpp_stanza_get_attribute attribute = size");
          attributes.size = 0;          
        } else {
          char *endptr; /* strtol endptr */
          long int size = strtol(size_attr, &endptr, 10);
          if (*endptr != '\0') {
            werr("strtol error: str = %s, val = %ld", size_attr, size);
            attributes.size = 0; 
          }

          attributes.size = size;
        }
      } else if (strncasecmp(type_attr, "file", 4) == 0) {
        attributes.valid = 1;
        attributes.type = REG;

        /* Get size attributes */
        char *size_attr = xmpp_stanza_get_attribute(stanza, "size");
        if (size_attr == NULL) {
          werr("xmpp_stanza_get_attribute attribute = size");
          attributes.size = 0;          
        } else {
          char *endptr; /* strtol endptr */
          long int size = strtol(size_attr, &endptr, 10);
          if (*endptr != '\0') {
            werr("strtol error: str = %s, val = %ld", size_attr, size);
            attributes.size = 0; 
          }

          attributes.size = size;
        }
      } else {
        werr("Unknown type: %s", type_attr);
        attributes.valid = -1;
      }

    }

    gotattr = 1;
    rc = pthread_cond_signal(&cond);
    if (rc != 0) {
      werr("pthread_mutex_unlock");
      perror("pthread_mutex_unlock");
    }

    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_unlock");
      perror("pthread_mutex_unlock");
    }
  }

  /* list action */
  else if (strncasecmp(action_attr, "list", 4) == 0) {
    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_lock");
      perror("pthread_mutex_lock");
    }

    /* Set data */

    /* Get error attribute */
    char *error_attr = xmpp_stanza_get_attribute(stanza, "error"); /* action attribute */
    if (error_attr == NULL) {
      werr("xmpp_stanza_get_attribute attribute = error");
      return;
    }

    if (strncasecmp(error_attr, "0", 1) != 0) {
      wlog("Error in attributes: %s", error_attr);
    } else {
      xmpp_stanza_t *child = xmpp_stanza_get_children(stanza); /* Stanza children */
      while (child != NULL) {
        elem_t *elem = (elem_t *)malloc(sizeof(elem_t));
        if (elem == NULL) {
          werr("malloc");
          perror("malloc");
          return;
        }
        elem->next = NULL;

        char *name = xmpp_stanza_get_name(child);
        if (name == NULL) {
          werr("xmpp_stanza_get_name");
          return;
        }

        if (strcmp(name, "directory") == 0) {
          elem->type = DIR;
        } else if (strcmp(name, "file") == 0) {
          elem->type = REG;
        } else {
          werr("Unknown name: %s", name);
        }

        char *filename_attr = xmpp_stanza_get_attribute(child, "filename");
        if (filename_attr == NULL) {
          werr("xmpp_stanza_get_attribute filename");
          return;
        }

        elem->filename = strdup(filename_attr);
        if (elem->filename == NULL) {
          werr("strdup");
          perror("strdup");
        } 

        /* Add elem in list */
        if (root == NULL) {
          root = elem;
        } else {
          last->next = elem;
        }
        last = elem;

        child = xmpp_stanza_get_next(child);
      }
    }

    /* Data set */
    gotlist = 1;
    rc = pthread_cond_signal(&cond);
    if (rc != 0) {
      werr("pthread_cond_signal");
      perror("pthread_cond_signal");
    }

    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_unlock");
      perror("pthread_mutex_unlock");
    }
  }

  /* read action */
  else if (strncasecmp(action_attr, "read", 4) == 0) {
    rc = pthread_mutex_lock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_lock");
      perror("pthread_mutex_lock");
    }

    /* Set data */

    char *data_str = xmpp_stanza_get_text(stanza); /* data string */
    if(data_str == NULL) {
      wlog("NULL data");
    }

    /* Decode */
    int dec_size = strlen(data_str) * 3 / 4 + 1; /* decoded data length */
    uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded data */
    int rc = base64_decode(decoded, data_str, dec_size); /* decode */

    read_data = strdup((char *)decoded);
    if (read_data == NULL) {
      werr("strdup");
      perror("strdup");
    } 

    /* Data set */

    gotread = 1;
    rc = pthread_cond_signal(&cond);
    if (rc != 0) {
      werr("pthread_cond_signal");
      perror("pthread_cond_signal");
    }

    rc = pthread_mutex_unlock(&mutex);
    if (rc != 0) {
      werr("pthread_mutex_unlock");
      perror("pthread_mutex_unlock");
    }     
  }
}

#endif /* FILES */
