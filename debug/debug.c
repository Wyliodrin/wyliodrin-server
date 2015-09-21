/**************************************************************************************************
 * Debug module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef DEBUG



/*** INCLUDES ************************************************************************************/

#include <stdbool.h>   /* bool             */
#include <unistd.h>    /* sleep            */
#include <signal.h>    /* signal           */
#include <errno.h>     /* errno            */
#include <string.h>    /* strerror, strlen */
#include <pthread.h>   /* threads          */
#include <unistd.h>    /* fork and exec    */
#include <sys/types.h> /* waitpid          */
#include <sys/wait.h>  /* waitpid          */

#include <hiredis/hiredis.h>           /* redis */
#include <hiredis/async.h>             /* redis */
#include <hiredis/adapters/libevent.h> /* redis */

#include "../winternals/winternals.h"  /* logs and error */
#include "../wxmpp/wxmpp.h"            /* xmpp stuff     */
#include "debug.h"                     /* declarations   */



/*** GLOBAL VARIABLES ****************************************************************************/

static redisContext *rc = NULL;
static bool is_connetion_in_progress = false;



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *owner_str;

extern xmpp_ctx_t *ctx;   /* from wxmpp/wxmpp.c */
extern xmpp_conn_t *conn; /* from wxmpp/wxmpp.c */



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static void *redis_connection_routine(void *args);
static void start_subscriber();
static void *subscriber_routine(void *arg);
static void connect_callback(const redisAsyncContext *rac, int status);
static void on_message(redisAsyncContext *rac, void *reply, void *privdata);
static void fork_and_exec_gdb();
static void *wait_routine(void *arg);



/*** IMPLEMENTATIONS *****************************************************************************/

void init_debug() {
  /* Start redis thread */
  pthread_t redis_thread;
  if (pthread_create(&redis_thread, NULL, redis_connection_routine, NULL) < 0) {
    werr("Could not start redis thread: %s", strerror(errno));
    return;
  }
  pthread_detach(redis_thread);
}


void debug(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
           xmpp_conn_t *const conn, void *const userdata) {
  if (strncmp(from, owner_str, strlen(owner_str) != 0)) {
    werr("Ignore debug comming from %s", from);
    return;
  }

  /* Check redis connection */
  if (rc == NULL || rc->err != 0) {
    /* Should wait until redis gets online? */
    werr("Not connected to redis: %s",
         rc->err != 0 ? rc->errstr : "context is NULL");
    if (!is_connetion_in_progress) {
      init_debug();
    }
    return;
  }

  /* New attribute */
  char *n_attr = xmpp_stanza_get_attribute(stanza, "n");
  if (n_attr != NULL) {
    fork_and_exec_gdb();
    usleep(500000);

    /* Send ACK */
    xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message_stz, "message");
    xmpp_stanza_set_attribute(message_stz, "to", owner_str);
    xmpp_stanza_t *ack_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(ack_stz, "d");
    xmpp_stanza_set_ns(ack_stz, WNS);
    xmpp_stanza_set_attribute(ack_stz, "n", "n");
    xmpp_stanza_add_child(message_stz, ack_stz);
    xmpp_send(conn, message_stz);
    xmpp_stanza_release(ack_stz);
    xmpp_stanza_release(message_stz);
  }

  char *d_attr = xmpp_stanza_get_attribute(stanza, "d");
  if (d_attr == NULL) {
    werr("There is no attribute named d in debug stanza");
    return;
  }

  redisReply *reply = redisCommand(rc, "PUBLISH %s %s", GDB_COMMANDS_CHANNEL, d_attr);
  if (reply == NULL) {
    werr("Failed to publish on channel %s the data %s because: %s",
      GDB_COMMANDS_CHANNEL, d_attr, rc->errstr);
  }
}



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void *redis_connection_routine(void *args) {
  struct timeval timeout = {1, 500000}; /* 1.5 seconds */

  is_connetion_in_progress = true;

  while (true) {
    rc = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
    if (rc == NULL || rc->err != 0) {
      werr("Redis connect error: %s",
           rc->err != 0 ? rc->errstr : "context is NULL");
      sleep(1);
    } else {
      start_subscriber();

      is_connetion_in_progress = false;
      return NULL;
    }
  }
}


static void  start_subscriber() {
  /* Start subscriber thread */
  pthread_t subscriber_thread;
  if (pthread_create(&subscriber_thread, NULL, subscriber_routine, NULL) < 0) {
    werr("Could not start subscriber thread: %s", strerror(errno));
    return;
  }
  pthread_detach(subscriber_thread);
}


static void *subscriber_routine(void *arg) {
  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  redisAsyncContext *rac = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
  if (rac == NULL || rac->err != 0) {
    werr("Could not establish an async connection to redis: %s",
      rac == NULL ? "context is NULL" : rac->errstr);
    return NULL;
  }

  redisLibeventAttach(rac, base);
  redisAsyncSetConnectCallback(rac, connect_callback);
  redisAsyncCommand(rac, on_message, NULL, "SUBSCRIBE %s", GDB_RESULTS_CHANNEL);
  event_base_dispatch(base);

  return NULL;
}


static void connect_callback(const redisAsyncContext *rac, int status) {
  if (status != REDIS_OK) {
    werr("Async connection to redis failed: %s", rac->errstr);
    return;
  }
  wlog("Async connection to redis succeeded");
}


static void on_message(redisAsyncContext *rac, void *reply, void *privdata) {
  if (reply == NULL) {
    werr("Got message with no reply");
    return;
  }

  redisReply *r = reply;
  if (r->type == REDIS_REPLY_ARRAY &&
      r->elements == 3 &&
      strncmp(r->element[0]->str, "message", 7) == 0 &&
      r->element[2]->str != NULL &&
      strlen(r->element[2]->str) > 0) {

    /* Just send the received data to owner */
    xmpp_stanza_t *message_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(message_stz, "message");
    xmpp_stanza_set_attribute(message_stz, "to", owner_str);
    xmpp_stanza_t *debug_stz = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(debug_stz, "d");
    xmpp_stanza_set_ns(debug_stz, WNS);
    xmpp_stanza_set_attribute(debug_stz, "d", r->element[2]->str);
    xmpp_stanza_add_child(message_stz, debug_stz);
    xmpp_send(conn, message_stz);
    xmpp_stanza_release(debug_stz);
    xmpp_stanza_release(message_stz);
  }
}


static void fork_and_exec_gdb() {
  pid_t pid = fork();

  if (pid < 0) {
    werr("Fork for executing gdb failed: %s", strerror(errno));
    return;
  }

  if (pid == 0) { /* Child */
    char *const argv[] = {"gdb", "-q", "-x", "/etc/wyliodrin/debugger.py", NULL};
    execvp(argv[0], argv);

    werr("Start new debug process failed");
    exit(EXIT_FAILURE);
  }

  /* Parent */
  pid_t *pid_p = malloc(sizeof(pid_p));
  if (pid_p == NULL) {
    werr("malloc failed: %s", strerror(errno));
    return;
  }
  *pid_p = pid;
  pthread_t wait_thread;
  if (pthread_create(&wait_thread, NULL, wait_routine, pid_p) < 0) {
    werr("Could not start wait thread: %s", strerror(errno));
    return;
  }
  pthread_detach(wait_thread);
}


static void *wait_routine(void *arg) {
  pid_t pid = *((pid_t *)arg);
  free(arg);

  waitpid(pid, NULL, 0);
  return NULL;
}



#endif /* DEBUG */
