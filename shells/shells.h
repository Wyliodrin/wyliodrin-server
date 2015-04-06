/**************************************************************************************************
 * Shells module
 *************************************************************************************************/

#ifndef _SHELLS_H
#define _SHELLS_H

#ifdef SHELLS

extern const char *owner_str; /* owner_str from init.c */

#define MAX_SHELLS 255

typedef struct {
  uint8_t id;          /* Shell id */
  uint32_t request_id; /* Request id */
  uint32_t fdm;        /* PTY file descriptor */
  xmpp_conn_t *conn;   /* XMPP Connection */
  xmpp_ctx_t *ctx;     /* XMPP Context */
} shell_t;

void init_shells();

/**
 * Parse shells commands
 */
void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata);

/**
 * Open shell
 */
void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/**
 * Send shells open response
 */
void send_shells_open_response(xmpp_stanza_t *stanza, xmpp_conn_t *const conn,
    void *const userdata, int8_t success, int8_t id);

/**
 * Close shell
 */
void shells_close(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/**
 * Keys from shell
 */
void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

/**
 * Keys response
 */
void send_shells_keys_response(xmpp_conn_t *const conn, void *const userdata,
    char *data_str, int data_len, int shell_id);

/**
 * List
 */
void shells_list(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata);

#endif /* SHELLS */

#endif /* _SHELLS_H */
