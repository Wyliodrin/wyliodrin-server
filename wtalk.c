/**************************************************************************************************
 * WTalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <ctype.h>     /* tolower      */
#include <errno.h>     /* errno        */
#include <fcntl.h>     /* file stuff   */
#include <signal.h>    /* SIGTERM      */
#include <string.h>    /* string stuff */
#include <sys/stat.h>  /* mkdir        */
#include <sys/wait.h>  /* waitpid      */
#include <Wyliodrin.h> /* lw version   */

#include "winternals/winternals.h" /* logs and errs   */
#include "wjson/wjson.h"           /* json stuff      */
#include "wtalk.h"                 /* file paths      */
#include "wtalk_config.h"          /* version         */
#include "wxmpp/wxmpp.h"           /* xmpp connection */

/*************************************************************************************************/


/*** VARIABLES ***********************************************************************************/

/* Variables found in wyliodrin.json */
const char *jid_str;
const char *owner_str;
const char *mount_file_str;
const char *build_file_str;
const char *board_str;
const char *sudo_str;
const char *shell_cmd;
const char *run_cmd;

bool privacy = false; /* privacy value from wylliodrin.json */

bool is_fuse_available; /* fuse checker */

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/* Returns the boardtype or NULL in case of errors.
 * Return value must be freed.
 */
static char *get_boardtype();

/*************************************************************************************************/

/**
 * Check whether fuse is available or not by stat /dev/fuse.
 */
static void check_for_fuse() {
  if (strcmp(board_str, "raspberrypi") == 0) {
    is_fuse_available = system("sudo stat /dev/fuse > /dev/null 2>&1") == 0 ? true : false;
  } else {
    is_fuse_available = system("stat /dev/fuse > /dev/null 2>&1") == 0 ? true : false;
  }
}

static void create_running_projects_file_if_does_not_exist() {
  int open_rc;

  /* Try to open RUNNING_PROJECTS_PATH */
  open_rc = open(RUNNING_PROJECTS_PATH, O_RDONLY);

  /* Create RUNNING_PROJECTS_PATH if it does not exist */
  if (open_rc == -1) {
    open_rc = open(RUNNING_PROJECTS_PATH, O_CREAT | O_RDWR);
    if (open_rc == -1) {
      werr("Error while trying to create " RUNNING_PROJECTS_PATH);
    }
  }
}

static void signal_handler(int signum) {
  if (signum == SIGTERM) {
    exit(EXIT_SUCCESS);
  }
}



void wtalk() {
  /* Get boartype */
  char *boardtype = get_boardtype();
  werr2(boardtype == NULL, return, "Could not get boardtype");

  /* Build path of settings_<boardtype>.json */
  char settings_path[128];
  int snprintf_rc = snprintf(settings_path, 128, "%s%s.json", SETTINGS_PATH, boardtype);
  wsyserr2(snprintf_rc < 0, return, "Could not build the settings configuration file");
  werr2(snprintf_rc >= 128, return, "File path of settings configuration file too long");

  /* Now that the settings configuration file is built, free boardtype */
  free(boardtype);

  /* Get the content from the settings_<boardtype> file in a json_object */
  json_t *settings_json = file_to_json_t(settings_path); /* JSON object of settings_<boardtype> */
  wfatal(settings_json == NULL, "Invalid JSON in %s", settings_path);

  /* Get the command that should be run when starting a shell */
  shell_cmd = get_str_value(settings_json, "shell_cmd"); /* config_file value */
  if (shell_cmd == NULL) {
    shell_cmd = strdup("bash");
  }

  run_cmd = get_str_value(settings_json, "run");
  wfatal(run_cmd == NULL, "No entry named run in %s", settings_path);

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
    if (strcmp(board_str, "raspberrypi") == 0) {
      snprintf_rc = snprintf(umount_cmd, 127, "sudo umount -f %s", mount_file_str);
    } else {
      snprintf_rc = snprintf(umount_cmd, 127, "umount -f %s", mount_file_str);
    }
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
          snprintf_rc = snprintf(wifi_type, 15, "OPEN");
        } else {
          snprintf_rc = snprintf(wifi_type, 15, "WPA-PSK");
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
        wifi_pid = fork();

        /* Return if fork failed */
        if (wifi_pid == -1) {
          werr("Fork used for setup_wifi_rpi failed: %s", strerror(errno));
        }

        /* Child from fork */
        else if (wifi_pid == 0) {
          char *env[] = {"sudo", "setup_wifi_rpi", (char *)ssid_str, (char *)psk_str, NULL};
          execvp(env[0], env);

          werr("setup_wifi_rpi failed");
          exit(EXIT_FAILURE);
        }
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
      snprintf_rc = snprintf(to_write, 127, "nameserver %s", nameserver_str);
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

  winfo("Starting wyliodrin-server v%d.%d with libwyliodrin v%d.%d",
    WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR,
    get_version_major(), get_version_minor());

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



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static char *get_boardtype() {
  /* Open boardtype */
  int boardtype_fd = open(BOARDTYPE_PATH, O_RDONLY);
  wsyserr2(boardtype_fd == -1, return NULL, "Could not open %s", BOARDTYPE_PATH);

  /* Allocate space for return value */
  char *boardtype = calloc(64, sizeof(char));
  wsyserr2(boardtype == NULL, return NULL, "Could not calloc space for boardtype");

  /* Read the content of the boartype file */
  int read_rc = read(boardtype_fd, boardtype, 63);
  wsyserr2(read_rc == -1, return NULL, "Could not read from %s", BOARDTYPE_PATH);
  werr2(read_rc == 63, return NULL, "Board name too long in %s", BOARDTYPE_PATH);

  /* Close boardtype file */
  int close_rc = close(boardtype_fd);
  wsyserr2(close_rc == -1, return NULL, "Could not close %s", BOARDTYPE_PATH);

  return boardtype;
}

/*************************************************************************************************/
