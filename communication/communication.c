/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *
 * TODO sendMesage, post, xmpp
 *************************************************************************************************/

#ifdef COMMUNICATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strophe.h>
#include <pthread.h>
#include <jansson.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"
#include "../base64/base64.h"
#include "communication.h"

redisContext *c;

extern xmpp_ctx_t *ctx;
extern xmpp_conn_t *conn;

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
    redisReply *r = reply;
    int j;
    json_t *json_message;
    json_error_t json_error;

    if (reply == NULL) {
    	wlog("onMessage NULL reply");
    	return;
    }

    if (r->type == REDIS_REPLY_ARRAY) {
        if (r->elements < 4) {
            wlog("At least 4 elements required");
            return;
        }

        /* Get port. The channel name must be communication_server:port */
        char *port = strchr(r->element[2]->str, ':');
        wfatal(port == NULL, "No : in channel %s", r->element[2]->str);

        port = strdup((const char *)port + 1);
        wsyserr(port == NULL, "strdup");

        /* Get json message */
        json_message = json_loads(r->element[3]->str, 0, &json_error);
        if(json_message == NULL) {
            werr("Message received on subscription is not a valid json: %s", r->element[3]->str);
            return;
        }

        /* Get id */
        json_t *id_json_obj = json_object_get(json_message, "id");
        if (id_json_obj == NULL) {
            werr("No id field in received json");
            return;
        }
        if (!json_is_string(id_json_obj)) {
            werr("Field id is not a string");
            return;
        }
        const char *id_str = json_string_value(id_json_obj);

        /* Get data */
        json_t *data_json_obj = json_object_get(json_message, "data");
        if (data_json_obj == NULL) {
            werr("No data field in received json");
            return;
        }
        if (!json_is_string(data_json_obj)) {
            werr("Field data is not a string");
            return;
        }
        const char *data_str = json_string_value(data_json_obj);

        /* Encode data_str in base64 */
        char *encoded_data = malloc(BASE64_SIZE(strlen(data_str)));
        wsyserr(encoded_data == NULL, "malloc");
        encoded_data = base64_encode(encoded_data, BASE64_SIZE(strlen(data_str)), 
            (const unsigned char *)data_str, strlen(data_str));
        wfatal(encoded_data == NULL, "encoded_data failed");

        /* Send it */
        xmpp_stanza_t *message = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(message, "message");
        xmpp_stanza_set_attribute(message, "to", id_str);
        xmpp_stanza_t *communication = xmpp_stanza_new(ctx);
        xmpp_stanza_set_name(communication, "communication");
        xmpp_stanza_set_ns(communication, WNS);
        xmpp_stanza_set_attribute(communication, "port", port);

        xmpp_stanza_t *data_stz = xmpp_stanza_new(ctx);
        xmpp_stanza_set_text(data_stz, encoded_data);

        xmpp_stanza_add_child(communication, data_stz);
        xmpp_stanza_add_child(message, communication);
        xmpp_send(conn, message);
        xmpp_stanza_release(message);
    } else {
        werr("Got message on subscription different from REDIS_REPLY_ARRAY");
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

    /* Put it in json */
    json_t *root = json_object();

    json_object_set_new(root, "from", json_string(from));
    json_object_set_new(root, "data", json_string((char *)decoded));

	redisCommand(c, "PUBLISH %s:%s %s", PUB_CHANNEL, port_attr, json_dumps(root, 0));
}

#endif /* COMMUNICATION */
