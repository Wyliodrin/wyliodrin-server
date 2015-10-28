/**************************************************************************************************
 * XMPP implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <unistd.h>  /* usleep         */
#include <pthread.h> /* mutex and cond */

#include "wyliodrin_connect_config.h" /* wtalk version        */
#include <Wyliodrin.h>    /* libwyliodrin version */

#include "../winternals/winternals.h" /* logs and errs    */
#include "../cmp/cmp.h"               /* msgpack handling */
#include "../wredis/wredis.h"

#include "wxmpp.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define XMPP_PORT 5222 /* XMPP server port */

#define CONN_INTERVAL 1 /* Time in seconds between connection attempts */

/*************************************************************************************************/



/*** SHARED VARIABLES ****************************************************************************/

xmpp_ctx_t *global_ctx = NULL;   /* XMPP context    */
xmpp_conn_t *global_conn = NULL; /* XMPP connection */

bool is_xmpp_connection_set = false;
bool is_owner_online = false;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern char *owner;
extern char *board;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/**
 * Connection handler
 */
static void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                         xmpp_stream_error_t * const stream_error, void * const userdata);

/**
 * Ping handler
 */
static int ping_handler     (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza,
                             void *const userdata);

/**
 * Presence handler
 */
static int presence_handler (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza,
                             void *const userdata);

/**
 * Message handler
 */
static int message_handler  (xmpp_conn_t *const conn, xmpp_stanza_t *const stanza,
                             void *const userdata);

/*************************************************************************************************/



/*** API implementation **************************************************************************/

/* Connect to XMPP */
void xmpp_connect(const char *jid, const char *pass) {
  xmpp_initialize();

  #ifdef VERBOSE
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_DEBUG);
  #else
    xmpp_log_t *log = xmpp_get_default_logger(XMPP_LEVEL_WARN);
  #endif

  global_ctx = xmpp_ctx_new(NULL, log);
  werr2(global_ctx == NULL, xmpp_shutdown(); return, "Could not get XMPP context");

  global_conn = xmpp_conn_new(global_ctx);
  werr2(global_conn == NULL, xmpp_ctx_free(global_ctx); xmpp_shutdown(); return,
        "Could not create XMPP connection");

  /* Setup authentication information */
  xmpp_conn_set_jid(global_conn, jid);
  xmpp_conn_set_pass(global_conn, pass);

  /* Initiate connection in loop */
  while (1) {
    int conn_rc = xmpp_connect_client(global_conn, NULL, XMPP_PORT, conn_handler, global_ctx);
    if (conn_rc < 0) {
      werr("Attempt to connect to XMPP server failed. Retrying...");
      sleep(CONN_INTERVAL);
    } else {
      break;
    }
  }

  /* Enter the event loop */
  xmpp_run(global_ctx);

  /* Event loop should run forever */
  werr("XMPP event loop completed. Retrying to connect...");

  /* Cleaning */
  xmpp_conn_release(global_conn);
  xmpp_ctx_free(global_ctx);
  xmpp_shutdown();
  global_ctx = NULL;
  global_conn = NULL;

  /* Retry to connect */
  if (strcmp(board, "server") != 0) {
    sleep(CONN_INTERVAL);
    return xmpp_connect(jid, pass);
  }
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATION *************************************************************/

static void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status, const int error,
                         xmpp_stream_error_t * const stream_error, void * const userdata) {
  /* Update XMPP context and connection */
  global_ctx = (xmpp_ctx_t *)userdata;
  global_conn = conn;

  /* Connection success */
  if (status == XMPP_CONN_CONNECT) {
    winfo("XMPP connection established");
    is_xmpp_connection_set = true;

    /* Add ping handler */
    xmpp_handler_add(global_conn, ping_handler, "urn:xmpp:ping", "iq", "get", global_ctx);

    /* Add presence handler */
    xmpp_handler_add(global_conn, presence_handler, NULL, "presence", NULL, global_ctx);

    /* Add wyliodrin handler */
    xmpp_handler_add(global_conn, message_handler, WNS, "message", NULL, global_ctx);

    /* Send presence stanza:
     * <presence><priority>50</priority></presence>
     */
    xmpp_stanza_t *presence_stz = xmpp_stanza_new(global_ctx); /* presence stanza */
    xmpp_stanza_set_name(presence_stz, "presence");
    xmpp_stanza_t *priority_stz = xmpp_stanza_new (global_ctx); /* priority stanza */
    xmpp_stanza_set_name(priority_stz, "priority");
    xmpp_stanza_t *value_stz = xmpp_stanza_new(global_ctx); /* value stanza */
    xmpp_stanza_set_text(value_stz, "50");
    xmpp_stanza_add_child(priority_stz, value_stz);
    xmpp_stanza_add_child(presence_stz, priority_stz);
    xmpp_send(global_conn, presence_stz);
    xmpp_stanza_release(value_stz);
    xmpp_stanza_release(priority_stz);
    xmpp_stanza_release(presence_stz);

    /* Send subscribe stanza:
     * <presence type="subscribe" to="<owner>"/>
     */
    xmpp_stanza_t *subscribe_stz = xmpp_stanza_new(global_ctx); /* subscribe stanza */
    xmpp_stanza_set_name(subscribe_stz, "presence");
    xmpp_stanza_set_type(subscribe_stz, "subscribe");
    xmpp_stanza_set_attribute(subscribe_stz, "to", owner);
    xmpp_send(global_conn, subscribe_stz);
    xmpp_stanza_release(subscribe_stz);
  }

  /* Connection error */
  else {
    werr("XMPP connection error");
    is_xmpp_connection_set = false;

    is_owner_online = false;

    xmpp_stop(global_ctx);
  }
}


int ping_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  /* Update XMPP context and connection */
  global_ctx = (xmpp_ctx_t *)userdata;
  global_conn = conn;

  char *from_attr = xmpp_stanza_get_attribute(stanza, "from");
  werr2(from_attr == NULL, return 1, "Received ping without from attribute");

  char *id_attr = xmpp_stanza_get_id(stanza);
  werr2(id_attr == NULL, return 1, "Received ping without id attribute from %s", from_attr);

  /* Send pong stanza:
   * <iq id=<id> type="result" to="<from>"/>
   */
  xmpp_stanza_t *pong_stz = xmpp_stanza_new(global_ctx);
  xmpp_stanza_set_name(pong_stz, "iq");
  xmpp_stanza_set_type(pong_stz, "result");
  xmpp_stanza_set_id(pong_stz, id_attr);
  xmpp_stanza_set_attribute(pong_stz, "to", from_attr);
  xmpp_send(global_conn, pong_stz);
  xmpp_stanza_release(pong_stz);

  return 1;
}


int presence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  /* Update XMPP context and connection */
  global_ctx = (xmpp_ctx_t *)userdata;
  global_conn = conn;

  char *from_attr = xmpp_stanza_get_attribute(stanza, "from");
  werr2(from_attr == NULL, return 1, "Received presence stanza without from attribute");

  if (strncasecmp(owner, from_attr, strlen(owner)) != 0) {
    /* Ignore presence stanza received from someone else */
    return 1;
  }

  char *type = xmpp_stanza_get_type(stanza);

  if (type == NULL) {
    /* User is online:
     * <presence to=<jid> from=<owner>>
     *    <priority>50</priority>
     *    <status>Happily echoing your &lt;message/&gt; stanzas</status>
     *  </presence>
     */
    if (xmpp_stanza_get_child_by_name(stanza, "status") != NULL) {
      winfo("Owner is online");

      is_owner_online = true;

      /* Send version */
      char wmajor[4];
      char wminor[4];
      char lwmajor[4];
      char lwminor[4];

      snprintf(wmajor,  4, "%d", WYLIODRIN_CONNECT_VERSION_MAJOR);
      snprintf(wminor,  4, "%d", WYLIODRIN_CONNECT_VERSION_MINOR);
      snprintf(lwmajor, 4, "%d", get_version_major());
      snprintf(lwminor, 4, "%d", get_version_minor());

      xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
      xmpp_stanza_set_name(message_stz, "message");
      xmpp_stanza_set_attribute(message_stz, "to", owner);
      xmpp_stanza_t *version_stz = xmpp_stanza_new(global_ctx);
      xmpp_stanza_set_name(version_stz, "version");
      xmpp_stanza_set_ns(version_stz, WNS);
      xmpp_stanza_set_attribute(version_stz, "wmajor", wmajor);
      xmpp_stanza_set_attribute(version_stz, "wminor", wminor);
      xmpp_stanza_set_attribute(version_stz, "lwmajor", lwmajor);
      xmpp_stanza_set_attribute(version_stz, "lwminor", lwminor);
      xmpp_stanza_add_child(message_stz, version_stz);
      xmpp_send(global_conn, message_stz);
      xmpp_stanza_release(version_stz);
      xmpp_stanza_release(message_stz);
    }
  }

  /* This stanza is received once when the board is first added. Stanza example:
   * <presence to="<jid>" type="subscribe" from="<owner>"/>
   */
  else if (strcmp(type, "subscribe") == 0) {
    /* Send subscribed:
     * <presence type="subscribed" to=<owner>/>
     */
    xmpp_stanza_t *subscribed_stz = xmpp_stanza_new(global_ctx);
    xmpp_stanza_set_name(subscribed_stz, "presence");
    xmpp_stanza_set_attribute(subscribed_stz, "to", owner);
    xmpp_stanza_set_type(subscribed_stz, "subscribed");
    xmpp_send(global_conn, subscribed_stz);
    xmpp_stanza_release(subscribed_stz);
  }

  /* Owner becomes unavailable. Stanza example:
   *  <presence to=<jid> type="unavailable" from=<owner>/>
   */
  else if (strcmp(type, "unavailable") == 0) {
    winfo("Owner is unavailable");
    is_owner_online = false;
  }

  return 1;
}


int message_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza, void *const userdata) {
  /* Update XMPP context and connection */
  global_ctx = (xmpp_ctx_t *)userdata;
  global_conn = conn;

  char *from_attr = xmpp_stanza_get_attribute(stanza, "from");
  werr2(from_attr == NULL, return 1, "Received message without from attribute");

  char *to_attr = xmpp_stanza_get_attribute(stanza, "to");
  werr2(to_attr == NULL, return 1, "Received message without to attribute from %s", from_attr);

  /* Check for error type */
  char *type = xmpp_stanza_get_type(stanza);
  werr2(type != NULL && strncasecmp(type, "error", 5) == 0, return 1,
    "Got message with error type from %s", from_attr);

  /* Stanza to msgpack */
  char **attrs = malloc(32 * sizeof(char *));
  char msgpack_buf[1024] = {0};

  cmp_ctx_t cmp;
  cmp_init(&cmp, msgpack_buf, 1024);

  xmpp_stanza_t *child_stz = xmpp_stanza_get_children(stanza);
  while (child_stz != NULL) {
    char *name = xmpp_stanza_get_name(child_stz);
    char *text = xmpp_stanza_get_text(child_stz);
    int num_attrs = xmpp_stanza_get_attributes(child_stz, attrs, 32);

    werr2(!cmp_write_map(&cmp, 3),
          return 1,
          "cmp_write_map error: %s", cmp_strerror(&cmp));

    werr2(!cmp_write_str(&cmp, "n", 1),
          return 1,
          "cmp_write_str error: %s", cmp_strerror(&cmp));

    werr2(!cmp_write_str(&cmp, name, strlen(name)),
          return 1,
          "cmp_write_str error: %s", cmp_strerror(&cmp));

    werr2(!cmp_write_str(&cmp, "t", 1),
          return 1,
          "cmp_write_str error: %s", cmp_strerror(&cmp));

    if (text != NULL) {
      werr2(!cmp_write_str(&cmp, text, strlen(text)),
            return 1,
            "cmp_write_str error: %s", cmp_strerror(&cmp));
    } else {
      werr2(!cmp_write_str(&cmp, NULL, 0),
            return 1,
            "cmp_write_str error: %s", cmp_strerror(&cmp));
    }

    werr2(!cmp_write_str(&cmp, "a", 1),
          return 1,
          "cmp_write_str error: %s", cmp_strerror(&cmp));

    werr2(!cmp_write_array(&cmp, num_attrs),
          return 1,
          "cmp_write_array error: %s", cmp_strerror(&cmp));

    int i;
    for (i = 0; i < num_attrs; i++) {
      werr2(!cmp_write_str(&cmp, attrs[i], strlen(attrs[i])),
            return 1,
            "cmp_write_str error: %s", cmp_strerror(&cmp));
    }

    publish(msgpack_buf);

  //   /* Test read */
  //   uint32_t map_size;
  //   werr2(!cmp_read_map(&cmp, &map_size),
  //         return 1,
  //         "cmp_read_map error: %s", cmp_strerror(&cmp));
  //   werr2(map_size != 3, return 1, "map_size = %d", map_size);

  //   char str[32];
  //   uint32_t str_size;

  //   /* Read name */
  //   str_size = 32;
  //   werr2(!cmp_read_str(&cmp, str, &str_size),
  //         return 1,
  //         "cmp_read_str error: %s", cmp_strerror(&cmp));
  //   printf("name key = %s\n", str);

  //   str_size = 32;
  //   werr2(!cmp_read_str(&cmp, str, &str_size),
  //         return 1,
  //         "cmp_read_str error: %s", cmp_strerror(&cmp));
  //   printf("name value = %s\n", str);

  //   str_size = 32;
  //   werr2(!cmp_read_str(&cmp, str, &str_size),
  //         return 1,
  //         "cmp_read_str error: %s", cmp_strerror(&cmp));
  //   printf("text key = %s\n", str);

  //   str_size = 32;
  //   werr2(!cmp_read_str(&cmp, str, &str_size),
  //         return 1,
  //         "cmp_read_str error: %s", cmp_strerror(&cmp));
  //   printf("text value = %s\n", str);

  //   str_size = 32;
  //   werr2(!cmp_read_str(&cmp, str, &str_size),
  //         return 1,
  //         "cmp_read_str error: %s", cmp_strerror(&cmp));
  //   printf("attr key = %s\n", str);

  //   uint32_t array_size;
  //   werr2(!cmp_read_array(&cmp, &array_size),
  //         return 1,
  //         "cmp_read_array error: %s", cmp_strerror(&cmp));

  //   for (i = 0; i < array_size; i += 2) {
  //     str_size = 32;
  //     werr2(!cmp_read_str(&cmp, str, &str_size),
  //           return 1,
  //           "cmp_read_str error: %s", cmp_strerror(&cmp));
  //     printf("attr[%d] key = %s\n", i, str);

  //     str_size = 32;
  //     werr2(!cmp_read_str(&cmp, str, &str_size),
  //           return 1,
  //           "cmp_read_str error: %s", cmp_strerror(&cmp));
  //     printf("attr[%d] val = %s\n", i+1, str);
  //   }

    child_stz = xmpp_stanza_get_next(child_stz);
  }

  return 1;
}

/*************************************************************************************************/
