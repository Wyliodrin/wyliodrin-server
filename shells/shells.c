/**************************************************************************************************
 * Shells module
 *************************************************************************************************/

#include <strophe.h> /* Strophe XMPP stuff */
#include <strings.h> /* strncasecmp */
#include <string.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"           /* WNS */
#include "shells.h"                   /* shells module api */

#ifdef SHELLS

void shells(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
            xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells(%s, %s, %d, stanza)", from, to, error);

  const char *action = xmpp_stanza_get_attribute(stanza, "action");
  if(strncasecmp(action, "open", 4) == 0) {
    shells_open(stanza, conn, userdata);
  } else if(strncasecmp(action, "close", 5) == 0) {
    shells_close();
  } else if(strncasecmp(action, "keys", 4) == 0) {
    shells_keys(stanza, conn, userdata);
  }

  wlog("Return from shells");
}

void shells_open(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_open(...)");

  /* Send done */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *done = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(done, "shells");
  xmpp_stanza_set_ns(done, WNS);
  xmpp_stanza_set_attribute(done, "action", "open");
  xmpp_stanza_set_attribute(done, "response", "done");
  xmpp_stanza_set_attribute(done, "shellid", "0");
  xmpp_stanza_set_attribute(done, "request", 
    (const char *)xmpp_stanza_get_attribute(stanza, "request"));
  xmpp_stanza_add_child(message, done);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  wlog("Return from shells_open");
}

void shells_close() {
  wlog("shells_close()");

  wlog("Return from shells_close");
}

void shells_keys(xmpp_stanza_t *stanza, xmpp_conn_t *const conn, void *const userdata) {
  wlog("shells_keys(...)");

  char *data_str = xmpp_stanza_get_text(stanza); /* data string */
  if(data_str == NULL) {
    wlog("Return from shells_keys due to NULL data");
    return;
  }

  /* Send back keys */
  xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata; /* Strophe context */

  xmpp_stanza_t *message = xmpp_stanza_new(ctx); /* message with done */
  xmpp_stanza_set_name(message, "message");
  xmpp_stanza_set_attribute(message, "to", owner_str);
  xmpp_stanza_t *keys = xmpp_stanza_new(ctx); /* shells action done stanza */
  xmpp_stanza_set_name(keys, "shells");
  xmpp_stanza_set_ns(keys, WNS);
  xmpp_stanza_set_attribute(keys, "shellid", 
    (const char *)xmpp_stanza_get_attribute(stanza, "shellid"));
  xmpp_stanza_set_attribute(keys, "action", "keys");
  xmpp_stanza_t *data = xmpp_stanza_new(ctx); /* data */
  wlog("data_str = %s\n\n\n", data_str);
  xmpp_stanza_set_text(data, data_str);
  wlog("data get text = %s\n\n\n", xmpp_stanza_get_text(data));
  xmpp_stanza_add_child(keys, data);
  xmpp_stanza_add_child(message, keys);
  xmpp_send(conn, message);
  xmpp_stanza_release(message);

  wlog("Return from shells_keys");
}

#endif /* SHELLS */
