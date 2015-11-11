/**************************************************************************************************
 * Hypervisor
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "cmp/cmp.h"               /* msgpack handling */
#include "winternals/winternals.h" /* logs and errors  */
#include "shells/shells.h"

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define REDIS_HOST "127.0.0.1"
#define REDIS_PORT 6379

#define REDIS_PUB_CHANNEL "wconnhyp"

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static void connectCallback(const redisAsyncContext *c, int status);
static void onMessage(redisAsyncContext *c, void *reply, void *privdata);

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

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

      /* Test read */
      cmp_ctx_t cmp;
      cmp_init(&cmp, r->element[2]->str, strlen(r->element[2]->str));

      uint32_t array_size;
      werr2(!cmp_read_array(&cmp, &array_size),
            return,
            "cmp_read_array error: %s", cmp_strerror(&cmp));

      werr2(array_size < 2, return, "Received array with less than 2 values");

      char *str = NULL;
      werr2(!cmp_read_str(&cmp, &str),
            return,
            "cmp_read_str error: %s", cmp_strerror(&cmp));

      if (strncmp(str, "shells", 6) == 0) {
        shells(r->element[2]->str);
        free(str);
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
