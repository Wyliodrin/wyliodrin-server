/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifdef COMMUNICATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strophe.h>
#include <pthread.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../base64/base64.h"
#include "communication.h"

redisContext *c;

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = reply;
    int j;

    if (reply == NULL) {
    	wlog("onMessage NULL reply");
    	return;
    }

    if (r->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < r->elements; j++) {
            wlog("%u) %s\n", j, r->element[j]->str);
        }
    }
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        werr("connectCallback: %s", c->errstr);
        return;
    }
    wlog("REDIS connected");
}

void *start_subscriber_routine(void *arg) {
	redisAsyncContext *c;

    signal(SIGPIPE, SIG_IGN);
    struct event_base *base = event_base_new();

    c = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
    wfatal(c->err != 0, "redisAsyncConnect: %s", c->errstr);

    redisLibeventAttach(c, base);
    redisAsyncSetConnectCallback(c, connectCallback);
    redisAsyncCommand(c, onMessage, NULL, "PSUBSCRIBE %s", SUB_CHANNEL);
    event_base_dispatch(base);

    return NULL;
}

void start_subscriber() {
	pthread_t t;
	int rc;

    rc = pthread_create(&t, NULL, start_subscriber_routine, NULL); /* Read rc */
    wsyserr(rc < 0, "pthread_create");
}

void init_communication() {
	start_subscriber();

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
    wfatal(c == NULL, "redisConnectWithTimeout");
    wfatal(c->err != 0, "redisConnectWithTimeout: %s", c->errstr);
}

void communication(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
	xmpp_conn_t *const conn, void *const userdata) 
{
	wlog("communication");

	char *port_attr = xmpp_stanza_get_attribute(stanza, "port");
	wfatal(port_attr == NULL, "No port attribute in communication");

	char *data_str = xmpp_stanza_get_text(stanza); /* data string */
	if(data_str == NULL) {
		werr("NULL data");
		return;
	}

	/* Decode */
	int dec_size = strlen(data_str) * 3 / 4 + 1; /* decoded data length */
	uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded data */
	base64_decode(decoded, data_str, dec_size);

	redisCommand(c, "PUBLISH %s:%s %s", PUB_CHANNEL, port_attr, (char *) decoded);
}

#endif /* COMMUNICATION */
