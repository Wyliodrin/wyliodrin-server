/**************************************************************************************************
 * Hypervisor
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <stdio.h>
#include <pthread.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "winternals/winternals.h"

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

#define REDIS_PUB_CHANNEL "wconnhyp"

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static void start_subscriber();
static void *start_subscriber_routine(void *arg);
static void connectCallback(const redisAsyncContext *c, int status);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

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
    werr("connectCallback: %s", c->errstr);
    return;
  }
  wlog("REDIS connected");
}


static void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;

  if (reply == NULL) {
    wlog("onMessage NULL reply");
    return;
  }

  if (r->type == REDIS_REPLY_ARRAY) {

    int i;
    for (i = 0; i < r->elements; i++) {
      if (r->element[i]->str != NULL) {
        printf("%d) %s\n", i, r->element[i]->str);
      } else {
        printf("%d) %s\n", i, "null");
      }
    }
  } else {
    werr("Got message on subscription different from REDIS_REPLY_ARRAY");
  }
}

/*************************************************************************************************/


/*** MAIN*****************************************************************************************/

int main() {
  struct event_base *base = event_base_new();

  redisAsyncContext *c = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
  werr2(c == NULL || c->err != 0, return 1, "redisAsyncConnect error: %s", c->errstr);


  redisLibeventAttach(c, base);
  redisAsyncSetConnectCallback(c, connectCallback);
  redisAsyncCommand(c, onMessage, NULL, "SUBSCRIBE %s", REDIS_PUB_CHANNEL);
  event_base_dispatch(base);

  werr("Return from start_subscriber_routine");

  return 0;
}

/*************************************************************************************************/
