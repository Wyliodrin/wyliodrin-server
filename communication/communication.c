/**************************************************************************************************
 * Communication module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#ifdef COMMUNICATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <strophe.h>
#include <pthread.h>
#include <jansson.h>
#include <unistd.h>
#include <curl/curl.h>
#include <time.h>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#include "../winternals/winternals.h" /* logs and errs */
#include "../wxmpp/wxmpp.h"
#include "../base64/base64.h"

#include "../cmp/cmp.h"  /* msgpack handling */
#include "../libds/ds.h" /* hashmap          */

#include "communication.h"


redisContext *c = NULL;

extern xmpp_ctx_t *global_ctx;
extern xmpp_conn_t *global_conn;
extern const char *jid;
extern const char *owner;
extern bool is_xmpp_connection_set;

static bool is_connetion_in_progress = false;

static hashmap_p hm;
static hashmap_p action_hm;

time_t time_of_last_hypervior_msg = 0;

#ifdef USEMSGPACK
  #include <stdbool.h>
  #include "../cmp/cmp.h"

  #define STORAGESIZE 1024
  #define SBUFSIZE    512

  static uint32_t reader_offset = 0;
  static uint32_t writer_offset = 0;

  static bool string_reader(cmp_ctx_t *ctx, void *data, size_t limit) {
    if (reader_offset + limit > STORAGESIZE) {
      fprintf(stderr, "No more space available in string_reader\n");
      return false;
    }

    memcpy(data, ctx->buf + reader_offset, limit);
    reader_offset += limit;

    return true;
  }

  static size_t string_writer(cmp_ctx_t *ctx, const void *data, size_t count) {
    if (writer_offset + count > STORAGESIZE) {
      fprintf(stderr, "No more space available in string_writer\n");
      return 0;
    }

    memcpy(ctx->buf + writer_offset, data, count);
    writer_offset += count;

    return count;
  }
#endif

void onMessage(redisAsyncContext *c, void *reply, void *privdata) {
  redisReply *r = reply;

  if (reply == NULL) {
    wlog("onMessage NULL reply");
    return;
  }

  if (r->type == REDIS_REPLY_ARRAY) {
    if (r->elements < 4) {
      wlog("At least 4 elements required");
      return;
    }

    /* Get port. The channel name must be communication_server:port or communication_server:mp:port */
    /* r->element[2]->str is the channel where the message has been received */
    char *port;
    const char *id_str;
    const char *data_str;
    #ifdef USEMSGPACK
      char *mp = strchr(r->element[2]->str, ':');
      if (mp == NULL) {
        werr("There should be at least a \":\" in channel %s", r->element[2]->str);
        return;
      } if (strncmp(mp+1, "mp", 2) != 0) {
        werr("Unknown channel: %s", r->element[2]->str);
        return;
      }

      port = strchr(mp + 1, ':');
      if (port == NULL) {
        werr("There should be 2 \":\" in channel %s", r->element[2]->str);
        return;
      }
      port = strdup((const char *)port + 1);
      wsyserr(port == NULL, "strdup");

      reader_offset = 0;

      int i;
      cmp_ctx_t cmp;
      uint32_t map_size = 0;
      uint32_t string_size = 0;
      char sbuf[SBUFSIZE] = {0};
      char storage[STORAGESIZE] = {0};

      memcpy(storage, r->element[3]->str, strlen(r->element[3]->str));

      cmp_init(&cmp, storage, string_reader, string_writer);

      if (!cmp_read_map(&cmp, &map_size)) {
        werr("cmp_read_map: %s", cmp_strerror(&cmp));
        return;
      }

      if (map_size != 2) {
        werr("map_size should be 2 instead of %d", map_size);
        return;
      }

      for (i = 0; i < map_size; i++) {
        /* Read key */
        string_size = sizeof(sbuf);
        if (!cmp_read_str(&cmp, sbuf, &string_size)) {
          werr("cmp_read_str: %s", cmp_strerror(&cmp));
          return;
        }

        /* Read value */
        if (strncmp(sbuf, "id", 2) == 0) {
          string_size = sizeof(sbuf);
          if (!cmp_read_str(&cmp, sbuf, &string_size)) {
            werr("cmp_read_str: %s", cmp_strerror(&cmp));
            return;
          }

          id_str = strdup(sbuf);
          wsyserr(id_str == NULL, "strdup");
        } else if (strncmp(sbuf, "data", 4) == 0) {
          string_size = sizeof(sbuf);
          if (!cmp_read_str(&cmp, sbuf, &string_size)) {
            werr("cmp_read_str: %s", cmp_strerror(&cmp));
            return;
          }

          data_str = strdup(sbuf);
          wsyserr(data_str == NULL, "strdup");
        } else {
          werr("Unknown key: %s", sbuf);
        }
      }
    #else
      port = strchr(r->element[2]->str, ':');
      if (port == NULL) {
        werr("There should be a \":\" in channel %s", r->element[2]->str);
        return;
      }
      port = strdup((const char *)port + 1);
      wsyserr(port == NULL, "strdup");

      json_t *json_message;
      json_error_t json_error;

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
      id_str = json_string_value(id_json_obj);

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
      data_str = json_string_value(data_json_obj);
    #endif

    /* Encode data_str in base64 */
    char *encoded_data = malloc(BASE64_SIZE(strlen(data_str)));
    wsyserr(encoded_data == NULL, "malloc");
    encoded_data = base64_encode(encoded_data, BASE64_SIZE(strlen(data_str)),
      (const unsigned char *)data_str, strlen(data_str));
    werr2(encoded_data == NULL, return, "Could not encode");

    /* Send it */
    if (is_xmpp_connection_set) {
      xmpp_stanza_t *message = xmpp_stanza_new(global_ctx);
      xmpp_stanza_set_name(message, "message");
      xmpp_stanza_set_attribute(message, "to", id_str);
      xmpp_stanza_t *communication = xmpp_stanza_new(global_ctx);
      xmpp_stanza_set_name(communication, "communication");
      xmpp_stanza_set_ns(communication, WNS);
      xmpp_stanza_set_attribute(communication, "port", port);

      xmpp_stanza_t *data_stz = xmpp_stanza_new(global_ctx);
      xmpp_stanza_set_text(data_stz, encoded_data);

      xmpp_stanza_add_child(communication, data_stz);
      xmpp_stanza_add_child(message, communication);
      xmpp_send(global_conn, message);
      xmpp_stanza_release(message);
    }
  } else {
    werr("Got message on subscription different from REDIS_REPLY_ARRAY");
  }
}


void onWyliodrinMessage(redisAsyncContext *ac, void *reply, void *privdata) {
  redisReply *r = reply;
  if (reply == NULL) return;

  if (r->type == REDIS_REPLY_ARRAY) {
    if (strncmp (r->element[0]->str, "subscribe", 9) == 0) {
      wlog("Subscribed to wyliodrin");
      return;
    }

    if (r->elements < 3) {
      wlog("At least 3 elements required");
      return;
    }

    char *message = r->element[2]->str;
    char *projectId;

    #ifdef USEMSGPACK
      if (strncmp(message, "signalmp", 8) != 0) {
        werr("Ignore message: %s", message);
        return;
      }

      projectId = message + 9;
      if (projectId == NULL) {
        werr("No project id");
        return;
      }
    #else
      if (strncmp(message, "signal", 6) != 0) {
        werr("Ignore message: %s", message);
        return;
      }

      projectId = message + 7;
      if (projectId == NULL) {
        werr("No project id");
        return;
      }
    #endif

    char command[128];
    snprintf(command, 127, "LRANGE %s 0 500", projectId);

    redisReply *reply = redisCommand(c, command);
    if (reply != NULL) {
      if (reply->type == REDIS_REPLY_ARRAY) {
        if (reply->elements == 0) {
          wlog("No elements");
          return;
        }

        /* Build json to be sent */
        const char *data_to_send;
        json_t *json_to_send = json_object();
        json_object_set_new(json_to_send, "projectid", json_string(projectId));
        json_object_set_new(json_to_send, "gadgetid",  json_string(jid));

        #ifdef USEMSGPACK
          int i, j, k;
          cmp_ctx_t cmp;
          uint32_t map_size = 0;
          uint32_t map_size2 = 0;
          uint32_t string_size = 0;
          char sbuf[SBUFSIZE] = {0};
          char kbuf[SBUFSIZE] = {0};
          char vbuf[SBUFSIZE] = {0};
          char *storage = NULL;
          double d;

          json_t *aux, *signals;
          json_t *array = json_array();

          for (j = 0; j < reply->elements; j++) {
            aux = json_object();
            signals = json_object();

            if (storage != NULL) {
              free(storage);
            }
            wlog("reply->element[%d]->str = %s", j, reply->element[j]->str);
            storage = strdup(reply->element[j]->str);
            wsyserr(storage == NULL, "strdup");

            reader_offset = 0;
            cmp_init(&cmp, storage, string_reader, string_writer);

            if (!cmp_read_map(&cmp, &map_size)) {
              werr("cmp_read_map error: %s", cmp_strerror(&cmp));
              return;
            }

            for (i = 0; i < map_size; i++) {
              string_size = sizeof(sbuf);
              if (!cmp_read_str(&cmp, sbuf, &string_size)) {
                werr("cmp_read_map error: %s", cmp_strerror(&cmp));
                return;
              }

              if (strncmp(sbuf, "u", 1) == 0) { /* userid */
                string_size = sizeof(sbuf);
                if (!cmp_read_str(&cmp, sbuf, &string_size)) {
                  werr("cmp_read_str error: %s", cmp_strerror(&cmp));
                  return;
                }

                if (j == 0) {
                  json_object_set_new(json_to_send, "userid", json_string(sbuf));
                }
              } else if (strncmp(sbuf, "sg", 2) == 0) { /* timestamp */
                if (!cmp_read_map(&cmp, &map_size2)) {
                  werr("cmp_read_map error: %s", cmp_strerror(&cmp));
                  return;
                }

                for (k = 0; k < map_size2; k++) {
                  string_size = sizeof(kbuf);
                  if (!cmp_read_str(&cmp, kbuf, &string_size)) {
                    werr("cmp_read_str error: %s", cmp_strerror(&cmp));
                    return;
                  }

                  string_size = sizeof(vbuf);
                  if (!cmp_read_double(&cmp, &d)) {
                    werr("cmp_read_double error: %s", cmp_strerror(&cmp));
                    return;
                  }

                  json_object_set_new(signals, kbuf, json_real(d));
                }
                json_object_set_new(aux, "signals", signals);
              } else if (strncmp(sbuf, "s",  1) == 0) { /* session */
                string_size = sizeof(sbuf);
                if (!cmp_read_str(&cmp, sbuf, &string_size)) {
                  werr("cmp_read_str error: %s", cmp_strerror(&cmp));
                  return;
                }

                if (j == 0) {
                  json_object_set_new(json_to_send, "session", json_string(sbuf));
                }
              } else if (strncmp(sbuf, "ts", 2) == 0) { /* timestamp */

                string_size = sizeof(sbuf);
                if (!cmp_read_double(&cmp, &d)) {
                  werr("cmp_read_str error: %s", cmp_strerror(&cmp));
                  return;
                }

                json_object_set_new(aux, "timestamp", json_real(d));
              } else if (strncmp(sbuf, "t",  1) == 0) { /* text */
                string_size = sizeof(sbuf);
                if (!cmp_read_str(&cmp, sbuf, &string_size)) {
                  werr("cmp_read_str error: %s", cmp_strerror(&cmp));
                  return;
                }

                json_object_set_new(aux, "text", json_string(sbuf));
              } else {
                werr("Unknown key: %s", sbuf);
                return;
              }
            }
            json_array_append(array, aux);
          }
          json_object_set_new(json_to_send, "data", array);
          data_to_send = (const char*)json_dumps(json_to_send, 0);
        #else
          json_error_t json_error;
          json_t *json = json_loads(reply->element[0]->str, 0, &json_error);
          if(json == NULL) {
            werr("Not a valid json: %s", r->element[0]->str);
            return;
          }
          json_object_set_new(json_to_send, "userid",  json_object_get(json, "userid"));
          json_object_set_new(json_to_send, "session", json_object_get(json, "session"));
          int i;
          json_t *aux;
          json_t *array = json_array();
          for (i = 0; i < reply->elements; i++) {
            aux = json_loads(reply->element[i]->str, 0, &json_error);
            if (aux == NULL) {
              werr("%s is not a valid json, i = %d", reply->element[i]->str, i);
            } else {
              json_object_del(aux, "userid");
              json_object_del(aux, "session");
              json_array_append(array, aux);
            }
          }
          json_object_set_new(json_to_send, "data", array);
          data_to_send = (const char*)json_dumps(json_to_send, 0);
        #endif

        /* Send it via http */
        CURL *curl;
        CURLcode res;

        curl = curl_easy_init();
        if (curl == NULL) {
          werr("Curl init failed");
          return;
        }
        char *domain = strrchr(jid, '@');
        if (domain == NULL) {
          werr("jid does not contain an at: %s", jid);
          char static_domain[] = "projects.wyliodrin.com";
          domain = static_domain;
        } else {
          domain++;
        }
        char URL[URL_SIZE];
        snprintf(URL, URL_SIZE-1, "http://%s/signals/send", domain);
        curl_easy_setopt(curl, CURLOPT_URL, URL);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 50L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data_to_send);
        struct curl_slist *list = NULL;
        list = curl_slist_append(list, "Content-Type: application/json");
        list = curl_slist_append(list, "Connection: close");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
        #ifdef LOG
          curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        #endif
        res = curl_easy_perform(curl);

        /* Check for errors */
        if(res != CURLE_OK) {
          werr("curl_easy_perform() failed: %s", curl_easy_strerror(res));
        } else {
          char ltrim_command[512];
          snprintf(ltrim_command, 511, "LTRIM %s %d -1", projectId, (int)(reply->elements));
          redisCommand(c, ltrim_command);
        }

        /* always cleanup */
        curl_easy_cleanup(curl);
      }
      freeReplyObject(reply);
    } else {
      werr("redis reply null");
    }
  } else {
    werr("Got message on wyliodrin subscription different from REDIS_REPLY_ARRAY");
  }
}

void onHypervisorMessage(redisAsyncContext *ac, void *reply, void *privdata) {
  redisReply *r = reply;
  if (reply == NULL) return;

  if (r->type == REDIS_REPLY_ARRAY) {
    if (r->elements == 3 && strncmp(r->element[0]->str, "subscribe", 9) == 0) {
      winfo("Successfully subscribed to %s", r->element[1]->str);
      publish(HYPERVISOR_PUB_CHANNEL, "ping", 4);
    }

    /* Manage message */
    else if ((r->elements == 3 && strncmp(r->element[0]->str, "message", 7) == 0)) {
      time_of_last_hypervior_msg = time(NULL);

      /* Manage pong */
      if (r->element[2]->len == 4 && strcmp(r->element[2]->str, "pong") == 0) {
        if (is_xmpp_connection_set) {
          xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
          xmpp_stanza_set_name(message_stz, "message");
          xmpp_stanza_set_attribute(message_stz, "to", owner);
          xmpp_stanza_t *keepalive_stz = xmpp_stanza_new(global_ctx);
          xmpp_stanza_set_name(keepalive_stz, "keepalive");
          xmpp_stanza_set_ns(keepalive_stz, WNS);
          xmpp_stanza_add_child(message_stz, keepalive_stz);
          xmpp_send(global_conn, message_stz);
          xmpp_stanza_release(keepalive_stz);
          xmpp_stanza_release(message_stz);
        }

        return;
      }

      cmp_ctx_t cmp;
      cmp_init(&cmp, r->element[2]->str, r->element[2]->len);

      uint32_t map_size;
      werr2(!cmp_read_map(&cmp, &map_size),
            return,
            "cmp_read_map error: %s", cmp_strerror(&cmp));

      /* Build stanza */
      if (is_xmpp_connection_set) {
        xmpp_stanza_t *message_stz = xmpp_stanza_new(global_ctx);
        xmpp_stanza_set_name(message_stz, "message");
        xmpp_stanza_set_attribute(message_stz, "to", owner);
        xmpp_stanza_t *shells_stz = xmpp_stanza_new(global_ctx);
        xmpp_stanza_set_name(shells_stz, "shells");
        xmpp_stanza_set_ns(shells_stz, WNS);

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

          if (strcmp(key, "t") == 0) {
            xmpp_stanza_t *data_stz = xmpp_stanza_new(global_ctx);

            char *encoded_data = malloc(BASE64_SIZE(strlen(value)) * sizeof(char));
            wsyserr2(encoded_data == NULL, return, "Could not allocate memory for keys");
            encoded_data = base64_encode(encoded_data, BASE64_SIZE(strlen(value)),
              (const unsigned char *)value, strlen(value));
            werr2(encoded_data == NULL, return, "Could not encode keys data");

            xmpp_stanza_set_text(data_stz, encoded_data);
            xmpp_stanza_add_child(shells_stz, data_stz);
          } else {
            char *key_replacement = (char *)hashmap_get(hm, key);
            werr2(key_replacement == NULL, return, "No entry named %s in attribute hashmap", key);

            char *value_replacement;
            if (strncmp(key, "a", strlen("a")) == 0) {
              value_replacement = (char *)hashmap_get(action_hm, value);
              werr2(value_replacement == NULL, return, "No entry named %s in attribute action hashmap",
                    value);
            } else {
              value_replacement = value;
            }

            xmpp_stanza_set_attribute(shells_stz, key_replacement, value_replacement);
          }

          free(key);
          free(value);
        }

        xmpp_stanza_add_child(message_stz, shells_stz);
        xmpp_send(global_conn, message_stz);

        xmpp_stanza_release(shells_stz);
        xmpp_stanza_release(message_stz);
      }
    } else {
      werr("Strange redis reply");
    }
  } else {
    werr("Got message on wyliodrin subscription different from REDIS_REPLY_ARRAY");
  }
}

void connectCallback(const redisAsyncContext *c, int status) {
  if (status != REDIS_OK) {
    werr("connectCallback: %s", c->errstr);
    return;
  }
  wlog("REDIS connected");
}

void wyliodrinConnectCallback(const redisAsyncContext *c, int status) {
  if (status != REDIS_OK) {
    werr("connectCallback: %s", c->errstr);
    return;
  }
  wlog("REDIS connected");
}

void hypervisorConnectCallback(const redisAsyncContext *c, int status) {
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
  werr2(c->err != 0, return NULL, "redisAsyncConnect error: %s", c->errstr);

  redisLibeventAttach(c, base);
  redisAsyncSetConnectCallback(c, connectCallback);
  redisAsyncCommand(c, onMessage, NULL, "PSUBSCRIBE %s", SUB_CHANNEL);
  event_base_dispatch(base);

  return NULL;
}


void *start_wyliodrin_subscriber_routine(void *arg) {
  redisAsyncContext *c;

  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  c = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
  werr2(c->err != 0, return NULL, "redisAsyncConnect error: %s", c->errstr);

  redisLibeventAttach(c, base);
  redisAsyncSetConnectCallback(c, wyliodrinConnectCallback);
  redisAsyncCommand(c, onWyliodrinMessage, NULL, "SUBSCRIBE %s", WYLIODRIN_CHANNEL);
  event_base_dispatch(base);

  werr("Return from start_wyliodrin_subscriber_routine");

  return NULL;
}

void *start_hypervisor_subscriber_routine(void *arg) {
  redisAsyncContext *c;

  signal(SIGPIPE, SIG_IGN);
  struct event_base *base = event_base_new();

  c = redisAsyncConnect(REDIS_HOST, REDIS_PORT);
  werr2(c->err != 0, return NULL, "redisAsyncConnect error: %s", c->errstr);

  redisLibeventAttach(c, base);
  redisAsyncSetConnectCallback(c, hypervisorConnectCallback);
  redisAsyncCommand(c, onHypervisorMessage, NULL, "SUBSCRIBE %s", HYPERVISOR_SUB_CHANNEL);
  event_base_dispatch(base);

  werr("Return from start_wyliodrin_subscriber_routine");

  return NULL;
}

void start_subscriber() {
  pthread_t t;
  int rc;

  rc = pthread_create(&t, NULL, start_subscriber_routine, NULL); /* Read rc */
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(t);
}

void start_wyliodrin_subscriber() {
  pthread_t t;
  int rc;

  rc = pthread_create(&t, NULL, start_wyliodrin_subscriber_routine, NULL); /* Read rc */
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(t);
}

void start_hypervisor_subscriber() {
  pthread_t t;
  int rc;

  rc = pthread_create(&t, NULL, start_hypervisor_subscriber_routine, NULL); /* Read rc */
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(t);
}

void *init_communication_routine(void *args) {
  struct timeval timeout = {1, 500000}; /* 1.5 seconds */

  is_connetion_in_progress = true;

  while (1) {
    c = redisConnectWithTimeout(REDIS_HOST, REDIS_PORT, timeout);
    if (c == NULL || c->err != 0) {
      werr("redis connect error: %s", c != NULL ? c->errstr : "context is NULL");
      sleep(1);
    } else {
      start_subscriber();
      start_wyliodrin_subscriber();
      start_hypervisor_subscriber();

      is_connetion_in_progress = false;
      return NULL;
    }
  }
}

void init_communication() {
  hm = create_hashmap();

  hashmap_put(hm, "w",  "width",     strlen("width")     + 1);
  hashmap_put(hm, "h",  "height",    strlen("height")    + 1);
  hashmap_put(hm, "r",  "request",   strlen("request")   + 1);
  hashmap_put(hm, "a",  "action",    strlen("action")    + 1);
  hashmap_put(hm, "p",  "projectid", strlen("projectid") + 1);
  hashmap_put(hm, "s",  "shellid",   strlen("shellid")   + 1);
  hashmap_put(hm, "u",  "userid",    strlen("userid")    + 1);
  hashmap_put(hm, "c",  "code",      strlen("code")      + 1);
  hashmap_put(hm, "ru", "running",   strlen("running")   + 1);
  hashmap_put(hm, "re", "response",  strlen("response")  + 1);

  action_hm = create_hashmap();

  hashmap_put(action_hm, "o", "open",       strlen("open")       + 1);
  hashmap_put(action_hm, "c", "close",      strlen("close")      + 1);
  hashmap_put(action_hm, "k", "keys",       strlen("keys")       + 1);
  hashmap_put(action_hm, "s", "status",     strlen("status")     + 1);
  hashmap_put(action_hm, "p", "poweroff",   strlen("poweroff")   + 1);
  hashmap_put(action_hm, "d", "disconnect", strlen("disconnect") + 1);

  pthread_t t; /* Read thread */
  int rc_int;

  rc_int = pthread_create(&t, NULL, &(init_communication_routine), NULL);
  wsyserr(rc_int < 0, "pthread_create");

  pthread_detach(t);
}

void communication(const char *from, const char *to, int error, xmpp_stanza_t *stanza,
  xmpp_conn_t *const conn, void *const userdata)
{
  wlog("communication");

  /* Sanity checks */
  if (c == NULL || c->err != 0) {
    werr("Redis connection error");
    if (!is_connetion_in_progress) {
      init_communication();
    }
    return;
  }

  /* Get port attribute from the received stanza */
  char *port_attr = xmpp_stanza_get_attribute(stanza, "port"); /* port attribute */
  werr2(port_attr == NULL, return, "No port attribute in communication");

  /* Get the text from the received stanza */
  char *text = xmpp_stanza_get_text(stanza); /* text */
  if (text == NULL) {
    werr("communication message with no text");
    return;
  }

  /* Decode the text */
  int dec_size = strlen(text) * 3 / 4 + 1; /* decoded text length */
  uint8_t *decoded = (uint8_t *)calloc(dec_size, sizeof(uint8_t)); /* decoded text */
  base64_decode(decoded, text, dec_size);

  /* Form the JSON that will be published */
  char *to_publish = NULL; /* pointer to the data that will be published */
  #ifdef USEMSGPACK
    cmp_ctx_t cmp;
    char storage[STORAGESIZE] = {0};

    writer_offset = 0;

    cmp_init(&cmp, storage, string_reader, string_writer);

    if (!cmp_write_map(&cmp, 2)) {
      werr("cmp_write_map error: %s", cmp_strerror(&cmp));
    } else if (!cmp_write_str(&cmp, "f", 1)) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    } else if (!cmp_write_str(&cmp, from, strlen(from))) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    } else if (!cmp_write_str(&cmp, "d", 1)) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    } else if (!cmp_write_str(&cmp, (const char *)decoded, strlen((char *)decoded))) {
      werr("cmp_write_str error: %s", cmp_strerror(&cmp));
    } else {
      to_publish = storage;
    }
  #else
    json_t *json = json_object(); /* json object */
    json_object_set_new(json, "from", json_string(from));
    json_object_set_new(json, "data", json_string((char *)decoded));
    to_publish = json_dumps(json, 0);
    json_decref(json);
  #endif

  /* Publish */
  if (to_publish == NULL) {
    werr("to_publish is NULL");
  } else {
    redisReply *reply; /* command reply */
    reply = redisCommand(c, "PUBLISH %s:%s %s", PUB_CHANNEL, port_attr, to_publish);
    if (reply == NULL) {
      werr("Failed to PUBLISH on channel %s:%s the data %s. Redis error: %s",
        PUB_CHANNEL, port_attr, to_publish, c->errstr);
    }
    freeReplyObject(reply);
  }
}

void publish(const char* channel, const char *data, int data_len) {
  redisReply *reply = redisCommand(c, "PUBLISH %s %b", channel, data, data_len);
  werr2(reply == NULL, /* Do nothing */, "Redis publish error: %s", c->errstr);
  freeReplyObject(reply);
}

#endif /* COMMUNICATION */
