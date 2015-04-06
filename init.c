/**************************************************************************************************
 * Initialization stuff
 *************************************************************************************************/

#include <stdio.h>   /* printf */
#include <jansson.h> /* json_t stuff */
#include <string.h>  /* strcmp */
#include <unistd.h>  /* sleep */
#include <strophe.h> /* Strophe stuff */
#include <ctype.h>   /* tolower */

#include "winternals/winternals.h"
#include "wjson/wjson.h"
#include "wxmpp/wxmpp.h"

#define SLEEP_NO_CONFIG (1 * 60) /* 1 minute of sleep in case of no config file */

const char *owner_str; /* owner */

/**
 * Init wyliodrin client.
 *
 * Read and decode settings file, get location of config file, read and decode config file,
 * get jid and pass, connect to Wyliodrin XMPP server.
 *
 * RETURN
 *     0 : succes
 *    -1 : NULL settings JSON
 *    -2 : config_file value is not a string
 *    -3 : NULL config JSON
 *    -4 : No jid in config file
 *    -5 : jid value is not a string
 *    -6 : No password in config file
 *    -7 : password value is not a string
 *    -8 : No owner in config file
 *    -9 : onwer value is not a string
 */
int8_t init() {
  wlog("init()");

  /* Decode settings JSON */
  json_t *settings = decode_json_text(SETTINGS_PATH); /* Settings JSON */
  if (settings == NULL) {
    wlog("Return -1 due to NULL decoded settings JSON");
    return -1;
  }

  /* Check whether config_file exists or not */
  json_t *config_file = json_object_get(settings, "config_file"); /* config_file object */
  if (!json_is_string(config_file)) {
    json_decref(settings);

    wlog("Return -2 because config_file value is not a string");
    return -2;
  }
  const char *config_file_txt = json_string_value(config_file); /* config_file text */
  if (strcmp(config_file_txt, "") == 0) {
    json_decref(settings);

    /* Sleep and try again */
    wlog("Config file value is an empty string. Sleep and retry.");
    sleep(SLEEP_NO_CONFIG);
    return init();
  }

  /* Decode config JSON */
  json_t *config = decode_json_text(config_file_txt); /* config_file JSON */
  if (config == NULL) {
    json_decref(settings);

    wlog("Return -3 due to NULL decoded config JSON");
    return -3;
  }

  /* Get jid */
  json_t *jid = json_object_get(config, "jid"); /* jid json */
  if (jid == NULL) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -4 due to inexistent jid in config file or error");
    return -4;
  }
  if (!json_is_string(jid)) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -5 because jid value is not a string");
    return -5;
  }
  const char* jid_str  = json_string_value(jid); /* jid value */

  /* Get pass */
  json_t *pass = json_object_get(config, "password"); /* pass json */
  if (pass == NULL) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -6 due to inexistent password in config file or error");
    return -6;
  }
  if (!json_is_string(pass)) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -7 because password value is not a string");
    return -7;
  }
  const char* pass_str  = json_string_value(pass); /* password value */

  /* Get owner*/
  json_t *owner = json_object_get(config, "owner"); /* owner json */
  if (owner == NULL) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -8 due to inexistent owner in config file or error");
    return -8;
  }
  if (!json_is_string(owner)) {
    json_decref(settings);
    json_decref(config);

    wlog("Return -9 because password value is not a string");
    return -9;
  }
  owner_str = json_string_value(owner);

  /* Convert owner to lowercase */
  uint32_t i; /* browser */
  char *p = (char *)owner_str; /* remove const-ness of owner_str */
  for (i = 0; i < strlen(p); i++) {
    p[i] = tolower(p[i]);
  }

  /* Connect to Wyliodrin XMPP server */
  wxmpp_connect(jid_str, pass_str);

  /* Cleaning */
  json_decref(settings);
  json_decref(config);

  wlog("Return 0 successful initialization");
  return 0;
}

int main(int argc, char *argv[]) {
  init();

  return 0;
}
