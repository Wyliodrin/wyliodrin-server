/**************************************************************************************************
 * Redis connection
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <pthread.h> /* thread handlind */
#include <unistd.h>  /* sleep           */
#include <stdlib.h>  /* free            */

#include <hiredis/hiredis.h>           /* redis */
#include <hiredis/async.h>             /* redis */
#include <hiredis/adapters/libevent.h> /* redis */

#include "../cmp/cmp.h"               /* msgpack       */
#include "../winternals/winternals.h" /* logs and errs */

#include "../../libds/ds.h" /* hashmap */

#include "../shells/shells.h" /* shells */

#include "wredis.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

#define REDIS_PUB_CHANNEL "whypsrv"
#define REDIS_SUB_CHANNEL "wsrvhyp"

#define SLEEP_TIME_BETWEEN_CONNECTION_RETRY 1

/*************************************************************************************************/



/*** STATIC VARIABLIES ***************************************************************************/

static redisContext *c = NULL;
static pthread_mutex_t pub_mutex = PTHREAD_MUTEX_INITIALIZER;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static void *init_redis_routine(void *args);
static void start_subscriber();
static void *start_subscriber_routine(void *arg);
static void connectCallback(const redisAsyncContext *c, int status);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

/*************************************************************************************************/



/*** API IMPLEMENATATION *************************************************************************/

void init_redis() {
  pthread_t t;

  int pthread_create_rc = pthread_create(&t, NULL, init_redis_routine, NULL);
  werr2(pthread_create_rc < 0, return, "Could not create thread for init_redis_routine");

  pthread_detach(t);
}


void publish(const char *str, int data_len) {
  if (c == NULL || c->err != 0) {
    werr("Cannot publish because redis is not connected: %s",
      c != NULL ? c->errstr : "context is NULL");
    return;
  }

  pthread_mutex_lock(&pub_mutex);

  redisReply *reply = redisCommand(c, "PUBLISH %s %b", REDIS_PUB_CHANNEL, str, data_len);
  werr2(reply == NULL, /* Do nothing */, "Redis publish error: %s", c->errstr);
  freeReplyObject(reply);

  pthread_mutex_unlock(&pub_mutex);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void *init_redis_routine(void *args) {
  struct timeval timeout = {1, 500000}; /* 1.5 seconds */

  while (1) {
    c = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
    if (c == NULL || c->err != 0) {
      werr("redisConnectWithTimeout error: %s", c->err != 0 ? c->errstr : "context is NULL");
      sleep(SLEEP_TIME_BETWEEN_CONNECTION_RETRY);
    } else {
      winfo("Redis initialization success");
      publish("pong", 4);
      start_subscriber();
      return NULL;
    }
  }
}


static void start_subscriber() {
  pthread_t t;

  int pthread_create_rc = pthread_create(&t, NULL, start_subscriber_routine, NULL);
  werr2(pthread_create_rc < 0, return, "Could not create thread for start_subscriber_routine");

  pthread_detach(t);
}


static void *start_subscriber_routine(void *arg) {
  /* signal(SIGPIPE, SIG_IGN); */
  struct event_base *base = event_base_new();

  redisAsyncContext *c = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
  werr2(c == NULL || c->err != 0, return NULL, "redisAsyncConnect error: %s", c->errstr);

  redisLibeventAttach(c, base);
  redisAsyncSetConnectCallback(c, connectCallback);
  redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE %s", REDIS_SUB_CHANNEL);
  event_base_dispatch(base);

  werr("Return from start_subscriber_routine");

  return NULL;
}


static void connectCallback(const redisAsyncContext *c, int status) {
  if (status != REDIS_OK) {
    werr("connectCallback error: %s", c->errstr);
    return;
  }
  winfo("Successfully connected to redis");
}


static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;

  if (reply == NULL) {
    wlog("Redis conntetion error. Retrying to connect");
    init_redis();
    return;
  }

  if (r->type == REDIS_REPLY_ARRAY) {
    /* Manage subscription */
    if (r->elements == 3 && strncmp(r->element[0]->str, "subscribe", 9) == 0) {
      winfo("Successfully subscribed to %s", r->element[1]->str);

    }

    /* Manage message */
    if ((r->elements == 3 && strncmp(r->element[0]->str, "message", 7) == 0)) {
      /* Manage ping */
      if (r->element[2]->len == 4 && strcmp(r->element[2]->str, "ping") == 0) {
        publish("pong", 4);
        return;
      }

      hashmap_p hm = create_hashmap();

      cmp_ctx_t cmp;
      cmp_init(&cmp, r->element[2]->str, r->element[2]->len);

      uint32_t map_size;
      werr2(!cmp_read_map(&cmp, &map_size),
            return,
            "cmp_read_map error: %s", cmp_strerror(&cmp));

      int i;
      char *key = NULL;
      char *value = NULL;
      for (i = 0; i < map_size / 2; i++) {
        werr2(!cmp_read_str(&cmp, &key),
              return,
              "cmp_read_str error: %s", cmp_strerror(&cmp));

        werr2(!cmp_read_str(&cmp, &value),
              return,
              "cmp_read_str error: %s", cmp_strerror(&cmp));

        hashmap_put(hm, key, value, strlen(value) + 1);

        free(key);
        free(value);
      }
      shells(hm);
      destroy_hashmap(hm);
    }
  } else {
    werr("Got message on subscription different from REDIS_REPLY_ARRAY");
  }
}

/*************************************************************************************************/
