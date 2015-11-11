/**************************************************************************************************
 * Redis handling
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "../cmp/cmp.h"               /* msgpack handling */
#include "../winternals/winternals.h" /* logs and errs   */

#include "wredis.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

#define REDIS_PUB_CHANNEL "wconnhyp"
#define REDIS_HYP_CHANNEL "whypconn"

/*************************************************************************************************/



/*** STATIC VARIABLIES ***************************************************************************/

static redisContext *c = NULL;

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
  /* Start redis */
  pthread_t t;

  int pthread_create_rc = pthread_create(&t, NULL, &(init_redis_routine), NULL);
  werr2(pthread_create_rc < 0, return, "Could not create thread for init_redis_routine");

  pthread_detach(t);

  start_subscriber();
}


void publish(const char *str) {
  redisReply *reply = redisCommand(c, "PUBLISH %s %s", REDIS_PUB_CHANNEL, str);
  werr2(reply == NULL, /* Do nothing */, "Redis publish error: %s", c->errstr);
  freeReplyObject(reply);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void *init_redis_routine(void *args) {
  struct timeval timeout = {1, 500000}; /* 1.5 seconds */

  while (1) {
    c = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
    if (c == NULL || c->err != 0) {
      werr("Redis connect error: %s", c->err != 0 ? c->errstr : "context is NULL");
      sleep(1);
    } else {
      winfo("Redis initialization success");
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
  redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE %s", REDIS_PUB_CHANNEL);
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
    wlog("onMessage NULL reply");
    return;
  }

  if (r->type == REDIS_REPLY_ARRAY) {
    /* Manage subscription */
    if (r->elements == 3 && strncmp(r->element[0]->str, "subscribe", 9) == 0) {
      winfo("Successfully subscribed to %s", r->element[1]->str);
    }

    /* Manage message */
    if ((r->elements == 3 && strncmp(r->element[0]->str, "message", 7) == 0)) {
      winfo("message: %s", r->element[2]->str);
    }
  } else {
    werr("Got message on subscription different from REDIS_REPLY_ARRAY");
  }
}

/*************************************************************************************************/
