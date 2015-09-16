/**************************************************************************************************
 * WTalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: August 2015
 *************************************************************************************************/

#include <string.h>    /* string stuff */
#include <ctype.h>     /* string stuff */
#include <unistd.h>    /* file stuff   */
#include <fcntl.h>     /* file stuff   */
#include <signal.h>    /* SIGTERM      */
#include <sys/stat.h>  /* file stuff   */
#include <sys/types.h> /* file stuff   */
#include <sys/wait.h>  /* waitpid      */
#include <Wyliodrin.h> /* version      */

#include "winternals/winternals.h" /* logs and errs */
#include "wxmpp/wxmpp.h"           /* xmpp stuff    */
#include "wjson/wjson.h"           /* json stuff    */
#include "wtalk.h"                 /* file paths    */
#include "wtalk_config.h"          /* version       */



/* Variables found in wyliodrin.json */
const char *jid_str;
const char *owner_str;
const char *mount_file_str;
const char *build_file_str;
const char *board_str;
const char *sudo_str;
const char *shell_cmd;

bool privacy = false; /* privacy value from wylliodrin.json */

bool is_fuse_available; /* fuse checker */



/**
 * Check whether fuse is available or not by stat /dev/fuse.
 */
static void check_for_fuse()
{
  is_fuse_available = system("stat /dev/fuse") == 0 ? true : false;
}

static void create_running_projects_file_if_does_not_exist() {
  int open_rc;

  /* Try to open RUNNING_PROJECTS_PATH */
  open_rc = open(RUNNING_PROJECTS_PATH, O_RDONLY);

  /* Create RUNNING_PROJECTS_PATH if it does not exist */
  if (open_rc == -1) {
    open_rc = open(RUNNING_PROJECTS_PATH, O_CREAT | O_RDWR);
    werr2(open_rc == -1, "Error while trying to create " RUNNING_PROJECTS_PATH);
  }
}

static void wifi_rpi(const char *ssid, const char *psk) {
  int fd = open(RPI_WIFI_PATH, O_CREAT | O_RDWR);
  if (fd == -1) {
    werr("Could not open %s", RPI_WIFI_PATH);
    return;
  }

  char to_write[512];
  snprintf(to_write, 512,
    "network={\n"
    "\tssid=\"%s\"\n"
    "\tproto=WPA RSN\n"
    "\tscan_ssid=1\n"
    "\tkey_mgmt=WPA-PSK NONE\n"
    "\tpsk=\"%s\"\n"
    "}\n",
    ssid, psk);

  char to_read[512];
  to_read[0] = '\0';
  read(fd, to_read, 512);
  if (strncmp(to_write, to_read, strlen(to_write)) == 0) {
    /* Already up to date */
    close(fd);
    return;
  }

  write(fd, to_write, strlen(to_write));
  close(fd);

  system("/etc/init.d/networking restart");
}

static void signal_handler(int signum) {
  if (signum == SIGTERM) {
    exit(EXIT_SUCCESS);
  }
}



void wtalk()
{
  /* Get the type of board from the boardtype file */
  int boardtype_fd = open(BOARDTYPE_PATH, O_RDONLY); /* File descriptor of boardtype file */
  if (boardtype_fd == -1) {
    werr("There should be a file named boardtype in /etc/wyliodrin");
    return;
  }

  /* Get the content from boardtype */
  char boardtype[BOARDTYPE_MAX_LENGTH]; /* Content of boardtype */
  memset(boardtype, 0, BOARDTYPE_MAX_LENGTH);
  int read_rc = read(boardtype_fd, boardtype, BOARDTYPE_MAX_LENGTH); /* Return code of read */
  wsyserr(read_rc == -1, "read");

  /* Get the path of settings_<boardtype> file */
  char settings_path[SETTINGS_PATH_MAX_LENGTH]; /* Path of the settings file */
  int snprintf_rc = snprintf(settings_path, SETTINGS_PATH_MAX_LENGTH, SETTINGS_PATH "%s.json",
    boardtype); /* Return code of snprintf */
  wsyserr(snprintf_rc < 0, "snprintf");

  /* Get the content from the settings_<boardtype> file in a json_object */
  json_t *settings_json = file_to_json_t(settings_path); /* JSON object of settings_<boardtype> */
  wfatal(settings_json == NULL, "Invalid JSON in %s", settings_path);

  /* Get the command that should be run when starting a shell */
  shell_cmd = get_str_value(settings_json, "shell_cmd"); /* config_file value */
  if (shell_cmd == NULL) {
    shell_cmd = strdup("bash");
  }

  /* Get config_file value. This value contains the path to wyliodrin.json */
  const char *config_file_str = get_str_value(settings_json, "config_file"); /* config_file value */
  wfatal(config_file_str == NULL, "Wrong config_file value in %s", settings_path);

  /* Get the content from the wyliodrin.json file in a json object */
  json_t *config_json = file_to_json_t(config_file_str); /* config_file as JSON */
  wfatal(config_json == NULL, "Invalid JSON in %s", config_file_str);

  /* Get sudo command */
  sudo_str = get_str_value(settings_json, "sudo");
  if (sudo_str == NULL) {
    sudo_str = "";
  }

  /* Set privacy based on privacy value from wyliodrin.json (if exists) */
  json_t *privacy_json = json_object_get(config_json, "privacy");
  if (privacy_json != NULL && json_is_boolean(privacy_json) && json_is_true(privacy_json)) {
    privacy = true;
  }

  /* Get mountFile value. This value containts the path where the projects are to be mounted */
  mount_file_str = get_str_value(settings_json, "mountFile");
  wfatal(mount_file_str == NULL, "Wrong mountFile value in %s", settings_path);
  mount_file_str = strdup(mount_file_str);
  wsyserr(mount_file_str == NULL, "strdup");

  if (mount_file_str[0] != '/') {
    werr("mountFile value does not begin with \"/\": %s", mount_file_str);
    return;
  }

  /* Create mount file */
  if (mkdir(mount_file_str, 0755) != 0) {
    char *aux;
    char *p = (char *)mount_file_str;
    while (true) {
      p = strchr(p + 1, '/');
      if (p == NULL) { break; }
      aux = strdup(mount_file_str);
      aux[p - mount_file_str] = '\0';
      mkdir(aux, 0755);
      free(aux);
    }
    mkdir(mount_file_str, 0755);
  }

  /* Get buildFile value. This value containts the path where the projects are to be mounted */
  build_file_str = get_str_value(settings_json, "buildFile");
  wfatal(build_file_str == NULL, "Wrong buildFile value in %s", settings_path);
  build_file_str = strdup(build_file_str);
  wsyserr(build_file_str == NULL, "strdup");

  if (build_file_str[0] != '/') {
    werr("buildFile value does not begin with \"/\": %s", build_file_str);
    return;
  }

  /* Create build file */
  if (mkdir(build_file_str, 0755) != 0) {
    char *aux;
    char *p = (char *)build_file_str + 1;
    while (true) {
      p = strchr(p + 1, '/');
      if (p == NULL) { break; }
      aux = strdup(build_file_str);
      aux[p - build_file_str] = '\0';
      mkdir(aux, 0755);
      wlog("Creating: %s", aux);
      free(aux);
    }
    mkdir(build_file_str, 0755);
  }

  /* Get the board value */
  board_str = get_str_value(settings_json, "board");
  wfatal(board_str == NULL, "No non-empty board key of type string in %s", settings_path);
  board_str = strdup(board_str);
  wsyserr(board_str == NULL, "strdup");

  /* Get jid value from wyliodrin.json */
  jid_str = get_str_value(config_json, "jid");
  wfatal(jid_str == NULL, "No non-empty jid key of type string in %s", config_file_str);
  jid_str = strdup(jid_str);

  /* Get passwork value from wyliodrin.json */
  const char* password_str = get_str_value(config_json, "password");
  wfatal(password_str == NULL, "No non-empty password key of type string in %s", config_file_str);

  /* Get owner value from wyliodrin.json */
  owner_str = get_str_value(config_json, "owner");
  wfatal(owner_str == NULL, "No non-empty owner key of type string in %s", config_file_str);
  owner_str = strdup(owner_str);
  wfatal(owner_str == NULL, "strdup");

  /* Convert owner to lowercase */
  int i; /* browser */
  char *p = (char *)owner_str; /* remove const-ness of owner_str */
  for (i = 0; i < strlen(p); i++) {
    p[i] = tolower(p[i]);
  }

  /* Umount the mountFile */
  if (strcmp(board_str, "server") != 0) {
    char umount_cmd[128];
    snprintf_rc = snprintf(umount_cmd, 128, "umount -f %s", mount_file_str);
    wsyserr(snprintf_rc < 0, "snprintf");
    int system_rc = system(umount_cmd);
    wsyserr(system_rc == -1, "system");

    check_for_fuse();
  } else {
    is_fuse_available = false;
  }

  /* Configure wifi of Edison boards */
  int wifi_pid = -1; /* Pid of fork's child in which edison's wifi configuration is done */
  if (strcmp(boardtype, "edison") == 0) {
    const char *ssid_str = get_str_value(config_json, "ssid");

    if (ssid_str != NULL && strlen(ssid_str) != 0) {
      const char *psk_str = get_str_value(config_json, "psk");
      if (psk_str != NULL) {
        /* Set wifi type: OPEN or WPA-PSK */
        char wifi_type[16];
        if (strlen(psk_str) == 0) {
          snprintf_rc = snprintf(wifi_type, 16, "OPEN");
        } else {
          snprintf_rc = snprintf(wifi_type, 16, "WPA-PSK");
        }
        wsyserr(snprintf_rc < 0, "snprintf");

        /* Fork and exec configure_edison */
        wifi_pid = fork();
        wfatal(wifi_pid == -1, "fork");
        if (wifi_pid == 0) { /* Child */
          char *args[] = {"configure_edison", "--changeWiFi",
            wifi_type, (char *)ssid_str, (char *)psk_str, NULL};
          execvp(args[0], args);
          werr("configure_edison failed");
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  /* Wifi for rpi */
  if (strcmp(boardtype, "raspberrypi") == 0) {
    const char *ssid_str = get_str_value(config_json, "ssid");

    if (ssid_str != NULL && strlen(ssid_str) != 0) {
      const char *psk_str = get_str_value(config_json, "psk");
      if (psk_str != NULL) {
        wifi_rpi(ssid_str, psk_str);
      }
    }
  }

  /* Update /etc/resolv.conf if nameserver is a valid entry in wyliodrin.json */
  const char *nameserver_str = get_str_value(config_json, "nameserver");
  if (nameserver_str != NULL && strlen(nameserver_str) != 0) {
    int resolv_fd = open("/etc/resolv.conf", O_WRONLY | O_TRUNC);
    if (resolv_fd < 0) {
      werr("Could not open resolv.conf");
    } else {
      char to_write[128];
      snprintf_rc = snprintf(to_write, 128, "nameserver %s", nameserver_str);
      wsyserr(snprintf_rc < 0, "snprintf");
      int write_rc = write(resolv_fd, to_write, strlen(to_write));
      wsyserr(write_rc == -1, "write");
    }
  }

  /* Create /wyliodrin directory */
  mkdir("/wyliodrin", 0755);

  /* Wait for wifi configuration */
  if (wifi_pid != -1) {
    waitpid(wifi_pid, NULL, 0);
  }

  create_running_projects_file_if_does_not_exist();

  /* Connect to XMPP server */
  xmpp_connect(jid_str, password_str);

  /* Cleaning */
  json_decref(config_json);
  json_decref(settings_json);
}

int main(int argc, char *argv[]) {
  /* Catch SIGTERM */
  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    werr("Unable to catch SIGTERM");
  }

  wtalk();

  return 0;
}
