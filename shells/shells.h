/**************************************************************************************************
 * Shells module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#ifndef _SHELLS_H
#define _SHELLS_H

#ifdef SHELLS

#define MAX_SHELLS 256 /* Maximum number of shells */

typedef struct {
  int pid;           /* PID */
  int id;            /* Shell id */
  int request_id;    /* open request */
  int fdm;           /* PTY file descriptor */
  xmpp_conn_t *conn; /* XMPP Connection */
  xmpp_ctx_t *ctx;   /* XMPP Context */
  int close_request; /* close request */
  char *projectid;   /* projectid in case of make shell */
} shell_t;

/* Initialize with NULL shells_vector */
void init_shells();

/* Parse shells commands */
void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata);

/* Open shell */
void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/* Send shells open response */
void send_shells_open_response(xmpp_stanza_t *stanza, xmpp_conn_t *const conn,
    void *const userdata, int8_t success, int8_t id, bool running);

/* Close shell */
void shells_close(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/* Keys from shell */
void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/* Keys response */
void send_shells_keys_response(xmpp_conn_t *const conn, void *const userdata,
    char *data_str, int data_len, int shell_id);

/* List */
void shells_list(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/* Status */
void shells_status(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/* Poweroff */
void shells_poweroff();

#endif /* SHELLS */

#endif /* _SHELLS_H */
