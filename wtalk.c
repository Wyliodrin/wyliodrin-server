/**************************************************************************************************
 * WTalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: June 2015
 *************************************************************************************************/

#include <stdio.h>   /* printf */
#include <jansson.h> /* json_t stuff */
#include <string.h>  /* strcmp */
#include <unistd.h>  /* sleep */
#include <strophe.h> /* Strophe stuff */
#include <ctype.h>   /* tolower */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "winternals/winternals.h" /* logs and errs */
#include "wjson/wjson.h"           /* json handling */
#include "wxmpp/wxmpp.h"           /* xmpp handling */

#define SLEEP_NO_CONFIG (1 * 60) /* 1 minute of sleep in case of no config file */

#define BOARDTYPE_PATH "/etc/wyliodrin/boardtype"

const char *jid_str;        /* jid        */
const char *owner_str;      /* owner      */
const char *mount_file_str; /* mount file */
const char *build_file_str; /* build file */
const char *board_str;      /* board name */

/**
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
int8_t wtalk() {
  wlog("wtalk()");
  int wifi_pid = -1;

  system("fusermount -u /wyliodrin/projects/mount");
  usleep(1000000);

  int fd = open(BOARDTYPE_PATH, O_RDONLY);
  wsyserr(fd == -1, "open");

  char boardtype[32];
  memset(boardtype, 0 ,32);
  int rc = read(fd, boardtype, 32);
  wsyserr(rc == -1, "read");

  /* Get settings path */
  char settings_path[128];
  sprintf(settings_path, "%s%s.json", SETTINGS_PATH, boardtype);

  /* Get settings JSON */
  json_t *settings = file_to_json_t(settings_path); /* Settings JSON */
  if (settings == NULL) {
    wlog("Return -1 due to NULL decoded settings JSON");
    return -1;
  }

  /* Check whether config_file key exists or not */
  json_t *config_file = json_object_get(settings, "config_file"); /* config_file object */
  if (config_file == NULL) {
    wlog("Return -1 due to not existing config_file key");
    return -1;
  }
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
    return wtalk();
  }

  /* Get mount file path */
  json_t *mount_file = json_object_get(settings, "mountFile"); /* mountFile object */
  if (mount_file == NULL) {
    wlog("Return -1 due to not existing mountFile key");
    return -1;
  }
  if (!json_is_string(mount_file)) {
    json_decref(settings);

    wlog("Return -2 because mountFile value is not a string");
    return -2;    
  }
  mount_file_str = json_string_value(mount_file);

  /* Get board name */
  json_t *board = json_object_get(settings, "board"); /* board object */
  if (board == NULL) {
    wlog("Return -1 due to not existing board key");
    return -1;
  }
  if (!json_is_string(board)) {
    json_decref(settings);

    wlog("Return -2 because board value is not a string");
    return -2;    
  }
  board_str = json_string_value(board);

  /* Get build file path */
  json_t *build_file = json_object_get(settings, "buildFile"); /* mountFile object */
  if (build_file == NULL) {
    wlog("Return -1 due to not existing buildFile key");
    return -1;
  }
  if (!json_is_string(build_file)) {
    json_decref(settings);

    wlog("Return -2 because buildFile value is not a string");
    return -2;    
  }
  build_file_str = json_string_value(build_file);

  /* Decode config JSON */
  json_t *config = file_to_json_t(config_file_txt); /* config_file JSON */
  if (config == NULL) {
    json_decref(settings);

    wlog("Return -3 due to NULL decoded config JSON");
    return -3;
  }

  /* Configure wifi of Edison boards */
  if (strcmp(boardtype, "edison") == 0) {
    /* Get ssid */
    json_t *ssid = json_object_get(config, "ssid");
    if (ssid == NULL) {
      json_decref(settings);
      json_decref(config);

      werr("No ssid in wyliodrin.json");
      return -1;
    }
    if (!json_is_string(ssid)) {
      json_decref(settings);
      json_decref(config);

      werr("Value of ssid is not a string");
      return -1;
    }
    const char *ssid_str = json_string_value(ssid);

    /* Continue only if ssid is non-empty */
    if (strlen(ssid_str) != 0) {
      printf("#include <.h>\n");
      /* Get psk */
      json_t *psk = json_object_get(config, "psk");
      if (psk == NULL) {
        json_decref(settings);
        json_decref(config);

        werr("No psk in wyliodrin.json");
        return -1;
      }
      if (!json_is_string(psk)) {
        json_decref(settings);
        json_decref(config);

        werr("Value of psk is not a string");
        return -1;
      }
      const char *psk_str = json_string_value(psk);

      /* Set wifi type: OPEN or WPA-PSK */
      char *type;
      if (strlen(psk_str) == 0) {
        type = strdup("OPEN");
      } else {
        type = strdup("WPA-PSK");
      }

      /* Pad ssid and psk with parathesis */
      char ssid_pad[strlen(ssid_str) + 3];
      char psk_pad[strlen(psk_str) + 3];
      sprintf(ssid_pad, "\"%s\"", ssid_str);
      sprintf(psk_pad, "\"%s\"", psk_str);

      /* Fork and exec configure_edison */
      wifi_pid = fork();
      wfatal(wifi_pid == -1, "fork");
      if (wifi_pid == 0) { /* Child */
        char *args[] = {"configure_edison", "--changeWifi", type, ssid_pad, psk_pad};
        execvp(args[0], args);
        werr("Configure edison failed");
        exit(EXIT_FAILURE);
      }
    }
  }

  /* Get nameserver */
  json_t *nameserver = json_object_get(config, "nameserver");
  if (nameserver == NULL) {
    wlog("No nameserver in wyliodrin.json");
  } else {
    if (!json_is_string(nameserver)) {
      wlog("nameserver is not a string");
    } else {
      const char *nameserver_str = json_string_value(nameserver); /* jid value */

      int fd = open("/etc/resolv.conf", O_WRONLY | O_TRUNC);
      if (fd < 0) {
        werr("Could not open resolv.conf");
      } else {
        char to_write[128];
        sprintf(to_write, "nameserver %s", nameserver_str);
        write(fd, to_write, strlen(to_write));
      }
    }
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
  jid_str  = json_string_value(jid); /* jid value */

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

  /* Wait for wifi configuration */
  if (wifi_pid != -1) {
    waitpid(wifi_pid, NULL, 0);
  }

  /* Connect to Wyliodrin XMPP server */
  wxmpp_connect(jid_str, pass_str);

  /* Cleaning */
  json_decref(settings);
  json_decref(config);

  wlog("Return 0 successful wtalk initializatioN");
  return 0;
}

int main(int argc, char *argv[]) {
  wtalk();

  usleep (3000000);
  return 0;
}
