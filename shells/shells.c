/**************************************************************************************************
 * Shells module
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */

#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <pty.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* WNS */
#include "../base64/base64.h"         /* encode decode */
#include "shells.h"                   /* shells module api */
#include "shells_helper.h"            /* read routine */

#ifdef SHELLS

shell_t *shells_vector[MAX_SHELLS]; /* All shells */

/* Init shells */
void init_shells() {
  uint8_t i;

  for(i = 0; i < MAX_SHELLS; i++) {
    shells_vector[i] = NULL;
  }
}

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells(%s, %s, %d, stanza)", from, to, error);

  const char *action = xmpp_stanza_get_attribute(stanza, "action");
  if(strncasecmp(action, "open", 4) == 0) {
    shells_open(stanza, conn, userdata);
  } else if(strncasecmp(action, "close", 5) == 0) {
    shells_close(stanza, conn, userdata);
  } else if(strncasecmp(action, "keys", 4) == 0) {
    shells_keys(stanza, conn, userdata);
  } else if(strncasecmp(action, "list", 4) == 0) {
    shells_list(stanza, conn, userdata);
  }

  wlog("Return from shells");
}

void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_open(...)");

  int rc; /* Return code */
  pthread_t rt; /* Read thread */

  char *w_str = xmpp_stanza_get_attribute(stanza, "width");
  if (w_str == NULL) {
    werr("No width in shells_open");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }
  char *h_str = xmpp_stanza_get_attribute(stanza, "height");
  if (h_str == NULL) {
    werr("No height in shells_open");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }

  long int w = strtol(w_str, NULL, 10);
  if (w == 0) {
    werr("Wrong width: %s", w_str);
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }
  long int h = strtol(h_str, NULL, 10);
  if (h == 0) {
    werr("Wrong height: %s", h_str);
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }

  int fdm; /* Master part of pty */
  struct winsize ws = {h, w, 0, 0}; /* Window size */

  /* Fork */
  int pid = forkpty(&fdm, NULL, NULL, &ws);
  if (pid == -1) {
    werr("SYSERR forkpty");
    perror("forkpty");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }

  if (pid != 0) { /* Parent */
    uint8_t i;
    for (i = 0; i < MAX_SHELLS; i++) {
      if (shells_vector[i] == NULL) {
        shells_vector[i] = (shell_t *)malloc(sizeof(shell_t));
        shells_vector[i]->id = i;
        shells_vector[i]->request_id = atoi(xmpp_stanza_get_attribute(stanza, "request"));
        shells_vector[i]->fdm = fdm;
        shells_vector[i]->conn = conn;
        shells_vector[i]->ctx = (xmpp_ctx_t *)userdata;
        break;
      }
    }

    if(i == MAX_SHELLS) {
      werr("No shells left");
      send_shells_open_response(stanza, conn, userdata, FALSE, -1);
      return;
    }

    rc = pthread_create(&rt, NULL, &(read_thread), shells_vector[i]);
    if (rc < 0) {
      wlog("SYSERR pthread_create");
      perror("pthread_create");
      send_shells_open_response(stanza, conn, userdata, FALSE, -1);
      return;
    }

    send_shells_open_response(stanza, conn, userdata, TRUE, i);
    wlog("Return success from shells_open");    
    return;
  }

  else { /* Child */
    char *args[] = {"screen", "-dRR", "ses", NULL};
    execvp(args[0], args);
    
    /* Error */
    fprintf(stderr, "Could not execvp\n");
    wlog("SYSERR execvp");
    perror("execvp");
    send_shells_open_response(stanza, conn, userdata, FALSE, -1);
    return;
  }

  send_shells_open_response(stanza, conn, userdata, FALSE, -1);
  wlog("Return error from shells_open");
}

void send_shells_open_response(xmpp_stanza_t *stanza, xmpp_conn_t *const conn,
    void *const userdata, int8_t success, int8_t id){
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */
  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *done = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(done, "shells");
  xmpp_stanza_set_ns(done, WNS);
  xmpp_stanza_set_attribute(done, "action", "open");
  if (success == TRUE) {
    xmpp_stanza_set_attribute(done, "response", "done");
    char id_str[4];
    sprintf(id_str, "%d", id);
    xmpp_stanza_set_attribute(done, "shellid", id_str);
  } else {
    xmpp_stanza_set_attribute(done, "response", "error");
  }
  xmpp_stanza_set_attribute(done, "request", 
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_add_child(message, done);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);
}

void shells_close(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_close(...)");

  /* Send close */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with close */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *close = xmpp_stanza_new(ctx); /* close stanza */
  xmpp_stanza_set_name(close, "shells");
  xmpp_stanza_set_ns(close, WNS);
  xmpp_stanza_set_attribute(close, "shellid", "0");
  xmpp_stanza_set_attribute(close, "action", "close");
  xmpp_stanza_set_attribute(close, "request",
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_set_attribute(close, "code", "0");
  xmpp_stanza_add_child(message, close);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  wlog("Return from shells_close");
}

void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_keys(...)");

  char *data_str = xmpp_stanza_get_text(stanza); /* data string */
  if(data_str == NULL) {
    wlog("Return from shells_keys due to NULL data");
    return;
  }

  /* Decode */
  int dec_size = strlen(data_str) * 3 / 4 + 1; /* decoded data length */
  uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded data */
  int rc = base64_decode(decoded, data_str, dec_size); /* decode */

  char *shellid_str = xmpp_stanza_get_attribute(stanza, "shellid");
  if (shellid_str == NULL) {
    werr("No shellid id keys");
    return;
  }

  int shellid = atoi(shellid_str);
  if (shells_vector[shellid] == NULL) {
    werr("Got keys from not existent shell");
    return;
  }

  /* Send decoded data to screen */
  write(shells_vector[shellid]->fdm, decoded, rc);


  wlog("Return from shells_keys");
}

void send_shells_keys_response(xmpp_conn_t *const conn, void *const userdata,
    char *data_str, int data_len, int shell_id) {
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *keys = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(keys, "shells");
  xmpp_stanza_set_ns(keys, WNS);
  char shell_id_str[4];
  sprintf(shell_id_str, "%d", shell_id);
  xmpp_stanza_set_attribute(keys, "shellid", shell_id_str);
  xmpp_stanza_set_attribute(keys, "action", "keys");

  xmpp_stanza_t *data = xmpp_stanza_new(ctx); /* data */
  char *encoded_data = (char *)malloc(BASE64_SIZE(data_len));
  encoded_data = base64_encode(encoded_data, BASE64_SIZE(data_len), 
    (const unsigned char *)data_str, data_len);

  if (encoded_data == NULL) {
    werr("Could not encode\n");
    return;
  }

  xmpp_stanza_set_text(data, encoded_data);
  xmpp_stanza_add_child(keys, data);
  xmpp_stanza_add_child(message, keys);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  free(encoded_data);
}

void shells_list(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_list(...)");

  /* Send list */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with close */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *list = xmpp_stanza_new(ctx); /* close stanza */
  xmpp_stanza_set_name(list, "shells");
  xmpp_stanza_set_ns(list, WNS);
  xmpp_stanza_set_attribute(list, "action", "list");
  xmpp_stanza_set_attribute(list, "request",
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_t *project = xmpp_stanza_new(ctx); /* project stanza */
  xmpp_stanza_set_attribute(project, "projectid", "0");
  xmpp_stanza_add_child(list, project);
  xmpp_stanza_add_child(message, list);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  wlog("Return from shell_list");
}

#endif /* SHELLS */
