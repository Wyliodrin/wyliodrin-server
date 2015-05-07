/**************************************************************************************************
 * XMPP connection
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#include <strophe.h>                  /* Strophe XMPP stuff */

#include "../winternals/winternals.h" /* logs and errs */
#include "../libds/ds.h"              /* hashmap */
#include "wxmpp.h"                    /* wxmpp api */
#include "wxmpp_handlers.h"           /* handlers */

#ifdef SHELLS
  #include "../shells/shells.h"         /* shells module */
#endif

#ifdef FILES
  #define FUSE_USE_VERSION 30
  #include <fuse.h>                     /* fuse */
  #include "../files/files.h"           /* files module */
#endif

#ifdef PS
  #include "../ps/ps.h"                 /* ps module */
#endif

hashmap_p tags = NULL; /* tags hashmap */

xmpp_ctx_t *ctx;   /* Context    */
xmpp_conn_t *conn; /* Connection */

int8_t wxmpp_connect(const char *jid, const char *pass) {
  wlog("wxmpp_connect(%s, %s)", jid, pass);

  /* Initialize the Strophe library */
  xmpp_initialize();

  /* Get Strophe logger, context and connection */
  xmpp_log_t *log  = xmpp_get_default_logger(XMPP_LEVEL_DEBUG); /* Strophe logger */
  ctx  = xmpp_ctx_new(NULL, log); /* Strophe context */
  if(ctx == NULL) {
    xmpp_shutdown();

    wlog("Return -1 due to NULL Strophe context");
    return -1;
  }
  conn = xmpp_conn_new(ctx); /* Strophe connection */
  if(conn == NULL) {
    xmpp_ctx_free(ctx);
    xmpp_shutdown();

    wlog("Return -2 due to NULL Strophe connection");
    return -2;
  }

  /* Setup authentication information */
  xmpp_conn_set_jid(conn, jid);
  xmpp_conn_set_pass(conn, pass);

  /* Initiate connection */
  if(xmpp_connect_client(conn, NULL, WXMPP_PORT, wconn_handler, ctx) < 0) {
    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);
    xmpp_shutdown();

    wlog("Return -3 due to connection error to XMPP server");
    return -3;
  }

  /* Create tags hashmap */
  tags = create_hashmap();

  /* Add modules */
  #ifdef SHELLS
    wadd_tag("shells", shells);
  #endif

  #ifdef FILES
    wadd_tag("files", files);
  #endif

  #ifdef PS
    wadd_tag("ps", ps);
  #endif

  /* Enter the event loop */
  xmpp_run(ctx);

  /* Cleaning */
  xmpp_conn_release(conn);
  xmpp_ctx_free(ctx);
  xmpp_shutdown();
  destroy_hashmap(tags);
  tags = NULL;

  /* Retry to connect */
  wlog("Retry to connect");
  wxmpp_connect(jid, pass);

  wlog("Retun 0 on success");
  return 0;
}

void wadd_tag(char *tag, tag_function f) {
  hashmap_put(tags, tag, &f, sizeof(void *));
}
