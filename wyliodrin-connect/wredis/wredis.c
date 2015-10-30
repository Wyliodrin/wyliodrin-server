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

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *board;
extern const char *home;
extern const char *mount_file;
extern const char *build_file;
extern const char *shell;
extern const char *run;
extern const char *stop;

extern const char *jid;
extern const char *owner;
extern bool privacy;

extern bool is_fuse_available;

/*************************************************************************************************/



/*** STATIC VARIABLIES ***************************************************************************/

static redisContext *c = NULL;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

static void *init_redis_routine(void *args);

/*************************************************************************************************/



/*** API IMPLEMENATATION *************************************************************************/

void init_redis() {
  pthread_t t;

  int pthread_create_rc = pthread_create(&t, NULL, &(init_redis_routine), NULL);
  werr2(pthread_create_rc < 0, return, "Could not create thread for init_redis_routine");

  pthread_detach(t);
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

      /* Send init info */


      return NULL;
    }
  }
}

/*************************************************************************************************/
