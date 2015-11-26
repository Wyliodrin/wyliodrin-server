/**************************************************************************************************
 * XMPP implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <unistd.h>  /* usleep         */
#include <pthread.h> /* mutex and cond */

#include "wtalk_config.h" /* wtalk version        */

#include "../libds/ds.h"              /* hashmap         */
#include "../winternals/winternals.h" /* logs and errs   */

/* Modules */
#include "../shells/shells.h"
#include "../files/files.h"
#include "../make/make.h"
#include "../communication/communication.h"
#include "../ps/ps.h"

#include "wxmpp.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define XMPP_PORT 5222 /* XMPP server port */

#define CONN_INTERVAL 1 /* Time in seconds between connection attempts */

/*************************************************************************************************/



/*** TYPEDEFS ************************************************************************************/

typedef void (*module_hander)(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
                              xmpp_conn_t *const conn, void *const userdata);

typedef struct {
  module_hander *handler;
  xmpp_stanza_t *stz;
  char *from_attr;
  char *to_attr;
} exec_handler_args_t;

/*************************************************************************************************/



/*** SHARED VARIABLES ****************************************************************************/

xmpp_ctx_t *global_ctx = NULL;   /* XMPP context    */
xmpp_conn_t *global_conn = NULL; /* XMPP connection */

bool is_xmpp_connection_set = false;
bool is_owner_online = false;

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static bool are_projects_initialized = false;
static hashmap_p modules = NULL;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern char *owner;
extern char *board;

extern bool is_fuse_available;

extern pthread_mutex_t mutex;
extern pthread_cond_t  cond;
extern bool signal_attr;
extern bool signal_list;
extern bool signal_read;
extern bool signal_fail;

extern int libwyliodrin_version_major;
extern int libwyliodrin_version_minor;

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

/**
 * Create modules hashmap
 */
static void create_modules_hashmap();

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

    /* Create modules hashmap on first connection */
    if (!are_projects_initialized) {
      create_modules_hashmap();
      are_projects_initialized = true;
    }

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

    /* Signal disconnect event to files module */
    int rc_int;
    rc_int = pthread_mutex_lock(&mutex);
    wsyserr(rc_int != 0, "pthread_mutex_lock");
    signal_attr = true;
    signal_list = true;
    signal_read = true;
    signal_fail = true;
    rc_int = pthread_cond_signal(&cond);
    wsyserr(rc_int != 0, "pthread_cond_signal");
    rc_int = pthread_mutex_unlock(&mutex);
    wsyserr(rc_int != 0, "pthread_mutex_unlock");

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


static int presence_handler(xmpp_conn_t *const conn, xmpp_stanza_t *const stanza,
                            void *const userdata) {
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

      snprintf(wmajor,  4, "%d", WTALK_VERSION_MAJOR);
      snprintf(wminor,  4, "%d", WTALK_VERSION_MINOR);
      snprintf(lwmajor, 4, "%d", libwyliodrin_version_major);
      snprintf(lwminor, 4, "%d", libwyliodrin_version_minor);

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

  /* Get every module function from stanza and execute it */
  module_hander *handler;
  xmpp_stanza_t *child_stz = xmpp_stanza_get_children(stanza);
  while (child_stz != NULL) {
    char *ns = xmpp_stanza_get_ns(child_stz);
    if (ns != NULL && strcmp(ns, WNS) == 0) {
      char *name = xmpp_stanza_get_name(child_stz);
      handler = (module_hander *)hashmap_get(modules, name);
      if (handler != NULL) {
        (*handler)(from_attr, to_attr, 0, child_stz, global_conn, global_ctx);
      } else {
        werr("Got message from %s that is trying to trigger unavailable module %s",
             from_attr, name);
      }
    }
    child_stz = xmpp_stanza_get_next(child_stz);
  }

  return 1;
}


static void create_modules_hashmap() {
  modules = create_hashmap();
  module_hander addr;

  #ifdef SHELLS
    addr = shells;
    hashmap_put(modules, "shells", &addr, sizeof(void *));
    init_shells();
  #endif
  #ifdef FILES
    if (is_fuse_available) {
      addr = files;
      hashmap_put(modules, "files", &addr, sizeof(void *));
      init_files();
    }
  #endif
  #ifdef MAKE
    addr = make;
    hashmap_put(modules, "make", &addr, sizeof(void *));
    init_make();
  #endif
  #ifdef COMMUNICATION
    addr = communication;
    hashmap_put(modules, "communication", &addr, sizeof(void *));
    init_communication();
  #endif
  #ifdef PS
    addr = ps;
    hashmap_put(modules, "ps", &addr, sizeof(void *));
  #endif
}

/*************************************************************************************************/
