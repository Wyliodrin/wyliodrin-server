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

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* stanzas       */
#include "../base64/base64.h"         /* decoe         */
#include "../libds/ds.h"              /* hashmap       */

#include "files.h"

extern xmpp_ctx_t *ctx;   /* Context    */
extern xmpp_conn_t *conn; /* Connection */

extern const char *owner_str; /* owner_str from init.c */
extern const char *mount_file_str; /* mount file */

extern bool is_owner_online; /* connection checker from wxmpp_handlers.c */

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; /* mutex      */
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;  /* condition  */

/* Condition signals */
bool signal_attr = false;
bool signal_list = false;
bool signal_read = false;
bool signal_fail = false;

/* Filetype */
typedef enum {
  DIR,          /* Directory    */
  REG           /* Regular file */
} filetype_t;

/* File attributes */
typedef struct {
  unsigned int size;
  filetype_t type;
  char is_valid;
} attr_t;

attr_t attributes = {0, DIR, -1};

/* List of files */
typedef struct elem_t {
  filetype_t type;
  char *filename;
  struct elem_t *next;
} elem_t;

/* root and last element of list of files */
elem_t *root = NULL;
elem_t *last = NULL;

char *read_data = NULL;

static void files_attr(hashmap_p h);
static void files_list(hashmap_p h);
static void files_read(hashmap_p h);

static int wfuse_getattr(const char *path, struct stat *stbuf) {
  wlog("wfuse_getattr path = %s\n", path);

  if (!is_owner_online) {
    return -ENOENT;
  }

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

  /* Send attributes stanza */
  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with files */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);

  xmpp_stanza_t *files = xmpp_stanza_new(ctx); /* files */
  xmpp_stanza_set_name(files, "files");
  xmpp_stanza_set_ns(files, WNS);
  xmpp_stanza_set_attribute(files, "action", "attributes");
  xmpp_stanza_set_attribute(files, "path", path);

  xmpp_stanza_add_child(message, files);
  xmpp_send(conn, message);
  xmpp_stanza_release(files);
  xmpp_stanza_release(message);

  /* Wait until attributes is set */
  while (signal_attr == false) {
    pthread_cond_wait(&cond, &mutex);
  }

  int res = 0;

  /* Do your job */
  if (signal_fail == true) {
    res = -ENOENT;

    signal_attr = false;
    signal_list = false;
    signal_read = false;
    signal_fail = false;
  } else {
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
      stbuf->st_mode = S_IFDIR | 0777;
      stbuf->st_nlink = 2;
    }

    else {
      if (attributes.is_valid == 1) {
        if (attributes.type == DIR) {
          stbuf->st_mode = S_IFDIR | 0777;
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
  }
  /* Job done */

  signal_attr = false;
  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");

  return res;
}

static int wfuse_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
  wlog("wfuse_readdir path = %s\n", path);

  if (!is_owner_online) {
    return -ENOENT;
  }

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

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
  xmpp_stanza_release(files);
  xmpp_stanza_release(message);

  while (signal_list == false) {
    pthread_cond_wait(&cond, &mutex);
  }

  int res = 0;

  /* Do your job */
  if (signal_fail == true) {
    res = -ENOENT;
    signal_attr = false;
    signal_list = false;
    signal_read = false;
    signal_fail = false;
  } else {
    // wfatal(root == NULL, "root is NULL");

    (void) offset;
    (void) fi;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    elem_t *aux = root;
    while (aux != NULL) {
      filler(buf, aux->filename, NULL, 0);
      aux = aux->next;
    }

    /* Free list */
    aux = root;
    elem_t *aux2;
    while (aux != NULL) {
      free(aux->filename);
      aux2 = aux;
      aux = aux->next;
      free(aux2);
    }

    root = NULL;
  }
  /* Job done */

  signal_list = false;
  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");

  return res;
}

static int wfuse_open(const char *path, struct fuse_file_info *fi) {
  wlog("wfuse_open path = %s\n", path);

  if (!is_owner_online) {
    return -ENOENT;
  }

  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  return 0;
}

static int wfuse_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
  wlog("wfuse_read path = %s\n", path);

  if (!is_owner_online) {
    return -ENOENT;
  }

  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

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
  xmpp_stanza_release(files);
  xmpp_stanza_release(message);

  /* Wait until attributes is set */
  while (signal_read == false) {
    pthread_cond_wait(&cond, &mutex);
  }

  int res = size;

  /* Do your job */
  if (signal_fail == true) {
    res = -ENOENT;
    signal_attr = false;
    signal_list = false;
    signal_read = false;
    signal_fail = false;
  } else {
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
  }
  /* Job done */

  signal_read = false;

  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");

  return res;
}

static struct fuse_operations wfuse_oper = {
  .getattr = wfuse_getattr,
  .readdir = wfuse_readdir,
  .open    = wfuse_open,
  .read    = wfuse_read
};

void *init_files_thread(void *a) {
  char *argv[] = {"dummy", "-s", "-f", (char *)mount_file_str};
  fuse_main(4, argv, &wfuse_oper, NULL);
  return NULL;
}

bool files_initialized = false;

void init_files() {
  if (!files_initialized) {
    pthread_t ift; /* Init files thread */
    int rc = pthread_create(&ift, NULL, init_files_thread, NULL);
    wsyserr(rc < 0, "pthread_create");

    files_initialized = true;
  }
}

void files(const char *from, const char *to, hashmap_p h) {
  wlog("files()");
  char *error = (char *)hashmap_get(h, "e");

  if (strcmp(error, "0") == 0) {
    char *action_attr = (char *)hashmap_get(h, "a"); /* action attribute */
    if (action_attr == NULL) {
      werr("xmpp_stanza_get_attribute attribute = action");
    }
    wfatal(action_attr == NULL, "xmpp_stanza_get_attribute [attribute = action]");

    if (strcmp(action_attr, "a") == 0) {
      files_attr(h);
    } else if (strcmp(action_attr, "l") == 0) {
      files_list(h);
    } else if (strncasecmp(action_attr, "r", 4) == 0) {
      files_read(h);
    } else {
      werr("Unknown action: %s", action_attr);
    }
  } else {
    werr("error stanza %s %s", (char *)hashmap_get(h, "p"), (char *)hashmap_get(h, "a"));
  }

  wlog("Return from files()");
}

static void files_attr(hashmap_p h) {
  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

  char *error_attr = (char *)hashmap_get(h, "e"); /* error attribute */
  wfatal(error_attr == NULL, "no error attribute in files stanza");

  if (strcmp(error_attr, "0") != 0) {
    wlog("Error in attributes: %s", error_attr);
    attributes.is_valid = -1;
  } else {
    char *type_attr = (char *)hashmap_get(h, "t"); /* type attribute */
    wfatal(type_attr == NULL, "no type attribute in files stanza");

    if (strcmp(type_attr, "d") == 0) {
      attributes.is_valid = 1;
      attributes.type = DIR;
    } else if (strcmp(type_attr, "f") == 0) {
      attributes.is_valid = 1;
      attributes.type = REG;
    } else {
      werr("Unknown type: %s", type_attr);
      attributes.is_valid = -1;
    }

    char *size_attr = (char *)hashmap_get(h, "s"); /* size */
    if (size_attr == NULL) {
      werr("xmpp_stanza_get_attribute attribute = size (%s)", (char *)hashmap_get(h, "p"));
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
  }

  signal_attr = true;

  rc = pthread_cond_signal(&cond);
  wsyserr(rc != 0, "pthread_cond_signal");

  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");
}

static void files_list(hashmap_p h) {
  int rc; /* Return code */

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

  char *error_attr = (char *)hashmap_get(h, "e"); /* error attribute */
  wfatal(error_attr == NULL, "no error attribute in files stanza");

  if (strcmp(error_attr, "0") != 0) {
    wlog("Error in attributes: %s", error_attr);
  } else {
    char *path_attr = (char *)hashmap_get(h, "p"); /* path attribute */
    wfatal(path_attr == NULL, "xmpp_stanza_get_attribute [attribute = path]");

    // xmpp_stanza_t *child = xmpp_stanza_get_children(stanza);
    // while (child != NULL) {
    //   elem_t *elem = (elem_t *)malloc(sizeof(elem_t));
    //   wsyserr(elem == NULL, "malloc");

    //   elem->next = NULL;

    //   char *name = xmpp_stanza_get_name(child);
    //   wfatal(name == NULL, "xmpp_stanza_get_name");

    //   if (strcmp(name, "directory") == 0) {
    //     elem->type = DIR;
    //   } else if (strcmp(name, "file") == 0) {
    //     elem->type = REG;
    //   } else {
    //     werr("Unknown name: %s", name);
    //   }

    //   char *filename_attr = xmpp_stanza_get_attribute(child, "filename");
    //   wfatal(filename_attr == NULL, "xmpp_stanza_get_attribute [attribute = filename]");

    //   elem->filename = strdup(filename_attr);
    //   wsyserr(elem->filename == NULL, "strdup");

    //   /* Add elem in list */
    //   if (root == NULL) {
    //     root = elem;
    //   } else {
    //     last->next = elem;
    //   }
    //   last = elem;

    //   child = xmpp_stanza_get_next(child);
    // }

    char *list_of_files = (char *)hashmap_get(h, "l");
    if (list_of_files == NULL) {
      werr("There is no list entry in files map");
    } else {
      char *saveptr;
      char *p = strtok_r(list_of_files, " ", &saveptr);
      while (p != NULL) {
        elem_t *elem = (elem_t *)malloc(sizeof(elem_t));
        wsyserr(elem == NULL, "malloc");

        elem->next = NULL;

        if (p[0] == 'd') {
          elem->type = DIR;
        } else if (p[0] == 'f') {
          elem->type = REG;
        } else {
          werr("Unknown name: %c", p[0]);
        }

        char *filename_attr = p + 1;

        elem->filename = strdup(filename_attr);
        wsyserr(elem->filename == NULL, "strdup");

        /* Add elem in list */
        if (root == NULL) {
          root = elem;
        } else {
          last->next = elem;
        }
        last = elem;

        p = strtok_r(NULL, " ", &saveptr);
      }
    }
  }

  /* Data set */
  signal_list = true;
  rc = pthread_cond_signal(&cond);
  wsyserr(rc != 0, "pthread_cond_signal");

  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");
}

static void files_read(hashmap_p h) {
  int rc;

  rc = pthread_mutex_lock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_lock");

  char *text = (char *)hashmap_get(h, "t");
  if(text == NULL) {
    werr("xmpp_stanza_get_text returned NULL (%s, error %s)", (char *)hashmap_get(h, "p"),
         (char *)hashmap_get(h, "e"));
    read_data = strdup ("");
  }
  else
  {
    int dec_size = strlen(text) * 3 / 4 + 1;
    uint8_t *dec_text = (uint8_t *)calloc(dec_size, sizeof(uint8_t));
    rc = base64_decode(dec_text, text, dec_size);
    wfatal(rc < 0, "base64_decode");

    if (read_data != NULL) {
      free(read_data);
    }
    read_data = strdup((char *)dec_text);
    wsyserr(read_data == NULL, "strdup");

    free(text);
    free(dec_text);
  }

  signal_read = true;
  rc = pthread_cond_signal(&cond);
  wsyserr(rc != 0, "pthread_cond_signal");

  rc = pthread_mutex_unlock(&mutex);
  wsyserr(rc != 0, "pthread_mutex_unlock");
}

#endif /* FILES */
