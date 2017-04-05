/**************************************************************************************************
 * WTalk
 *
 * Author: Ioana CULIC <ioana.culic@wyliodrin.com>
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#ifdef DEVICEINTEL
  #include <mraa.h>
#endif

#include <string.h>    /* string stuff */
#include <unistd.h>    /*open          */
#include <ctype.h>     /* tolower      */
#include <errno.h>     /* errno        */
#include <fcntl.h>     /* file stuff   */
#include <signal.h>    /* SIGTERM      */
#include <string.h>    /* string stuff */
#include <sys/stat.h>  /* mkdir        */
#include <sys/types.h> /* stat         */
#include <sys/wait.h>  /* waitpid      */
#include <unistd.h>    /* stat         */

#include "winternals/winternals.h" /* logs and errs   */
#include "wjson/wjson.h"           /* json stuff      */
#include "wtalk.h"                 /* file paths      */
#include "wtalk_config.h"          /* version         */
#include "wxmpp/wxmpp.h"           /* xmpp connection */

/*************************************************************************************************/




/*** VARIABLES ***********************************************************************************/

const char *board;
const char *logout_path;
const char *logerr_path;
int pong_timeout = DEFAULT_PONG_TIMEOUT;
static const char *config_file;

/* Values from wyliodrin.json */
const char *jid;
const char *owner;
static const char *password;
static const char *ssid;
static const char *psk;

bool privacy = false;
const char *nameserver;

FILE *log_out;
FILE *log_err;
/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/**
 * Returns the boardtype or NULL in case of errors.
 * Return value must be freed.
 */
static char *get_boardtype();

/**
 * Load values from settings file in separate variables. Return true if every expected value is
 * found in the settings file, or false otherwise.
 * settings_file is used to print error logs.
 */
static bool load_content_from_settings_file(json_t *settings_json, const char *settings_file);

/**
 * Load values from config file in separate variables. Return true if every expected value is
 * found in the config file, or false otherwise.
 * config_file is used to print error logs.
 */
static bool load_content_from_config_file(json_t *config_json, const char *config_file);

/*************************************************************************************************/



/*** MAIN ****************************************************************************************/

int main(int argc, char *argv[]) {
  log_out = stdout;
  log_err = stderr;

  winfo("Starting wyliodrind");

  /* Catch SIGTERM */
  if (signal(SIGTERM, signal_handler) == SIG_ERR) {
    werr("Unable to catch SIGTERM");
  }

  /* Get boardtype */
  board = get_boardtype();
  werr2(board == NULL, goto _finish, "Could not get type of board");

  /* Build path of settings_<boardtype>.json */
  char settings_file[128];
  int snprintf_rc = snprintf(settings_file, 128, "%s%s.json", SETTINGS_PATH, board);
  wsyserr2(snprintf_rc < 0, goto _finish, "Could not build the settings configuration file");
  werr2(snprintf_rc >= 128, goto _finish, "File path of settings configuration file too long");


  /* Get the content from the settings_<boardtype> file in a json_object */
  json_t *settings_json = file_to_json(settings_file);
  werr2(settings_json == NULL, goto _finish, "Could not load JSON from %s", settings_file);

  /* Load content from settings_<boardtype> in variables */
  bool load_settings_rc = load_content_from_settings_file(settings_json, settings_file);
  werr2(!load_settings_rc, goto _finish, "Invalid settings in %s", settings_file);


  //TODO
  /* Set local logs */
  // log_out = fopen(logout_path, "a");
  // if (log_out == NULL) { log_out = stdout; }
  // log_err = fopen(logerr_path, "a");
  // if (log_err == NULL) { log_err = stderr; }

  /* Load content from wyliodrin.json. The path to this file is indicated by the config_file
   * entry from the settings configuration file. */
  if (strncmp(board, "edison", strlen("edison")) == 0) {
    system("ls /media/storage/wyliodrin.json || "
           "(mkdir -p /media/storage ; mount -o loop,ro,offset=8192 /dev/mmcblk0p9 /media/storage)");
  }
  json_t *config_json = file_to_json(config_file);
  if (strncmp(board, "edison", strlen("edison")) == 0) {
    system("ls /media/storage/wyliodrin.json && umount /media/storage");
  }
  werr2(config_json == NULL, goto _finish, "Could not load JSON from %s", config_file);

  bool load_config_rc = load_content_from_config_file(config_json, config_file);
  werr2(!load_config_rc, goto _finish, "Invalid configuration in %s", config_file);

  //TODO
  // winfo("Starting wyliodrin-server v%d.%d with libwyliodrin v%d.%d",
  //   WTALK_VERSION_MAJOR, WTALK_VERSION_MINOR,
  //   libwyliodrin_version_major, libwyliodrin_version_minor);

  // winfo("Board = %s", board);
  // winfo("Owner = %s", owner);
  // winfo("Connecting to XMPP as %s", jid);

  //xmpp_connect(jid, password);

  // _finish: ;
  //   if (log_out != stdout) {
  //     fclose(log_out);
  //   }

  //   if (log_err != stderr) {
  //     fclose(log_err);
  //   }

    /* Let it sleep for a while for error messages to be sent */
    sleep(3);

    return 0;
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static char *get_boardtype() {
  #ifdef DEVICEINTEL
  	mraa_init ();
    const char* board = mraa_get_platform_name();
    if (board == NULL)
    {
      return "edison";
    }
    else
    if (strncmp (board, "Intel Galileo ", 14)==0)
    {
      return "arduinogalileo";
    }
    else
    if (strncmp (board, "Intel Edison", 12)==0)
    {
      return "edison";
    }
    else
    if (strncmp (board, "MinnowBoard MAX", 16)==0)
    {
      return "minnowboardmax";
    }
    else
    {
      return NULL;
    }
  #else
    char *return_value = NULL;

  /* Open boardtype */
  int boardtype_fd = open(BOARDTYPE_PATH, O_RDONLY);
  //wsyserr2(boardtype_fd == -1, goto _finish, "Could not open %s", BOARDTYPE_PATH);

  /* Allocate space for return value */
  char *boardtype = calloc(64, sizeof(char));
  //wsyserr2(boardtype == NULL, goto _finish, "Could not calloc space for boardtype");

  /* Read the content of the boardtype file */
  int read_rc = read(boardtype_fd, boardtype, 63);
  // wsyserr2(read_rc == -1, goto _finish, "Could not read from %s", BOARDTYPE_PATH);
  // werr2(read_rc == 63, goto _finish, "Board name too long in %s", BOARDTYPE_PATH);

  /* Success */
  return_value = boardtype;

  // _finish: ;
  //   if (boardtype_fd != -1) {
  //     int close_rc = close(boardtype_fd);
  //     wsyserr2(close_rc == -1, return_value = NULL,
  //              "Could not close %s", BOARDTYPE_PATH);
  //   }

    return return_value;
    #endif
}

static bool load_content_from_settings_file(json_t *settings_json, const char *settings_file) {
  bool return_value = false;

  config_file = get_str_value(settings_json, "config_file");
  werr2(config_file == NULL, goto _finish, "There is no config_file entry in %s", settings_file);

  home = get_str_value(settings_json, "home");
  werr2(home == NULL, goto _finish, "There is no home entry in %s", settings_file);

  mount_file = get_str_value(settings_json, "mount_file");
  werr2(mount_file == NULL, goto _finish, "There is no mount_file entry in %s", settings_file);

  build_file = get_str_value(settings_json, "build_file");
  werr2(build_file == NULL, goto _finish, "There is no build_file entry in %s", settings_file);

  shell = get_str_value(settings_json, "shell");
  werr2(shell == NULL, goto _finish, "There is no shell entry in %s", settings_file);

  run = get_str_value(settings_json, "run");
  werr2(run == NULL, goto _finish, "There is no run entry in %s", settings_file);

  stop = get_str_value(settings_json, "stop");
  werr2(stop == NULL, goto _finish, "There is no stop entry in %s", settings_file);

  poweroff = get_str_value(settings_json, "poweroff");
  werr2(poweroff == NULL, goto _finish, "There is no poweroff entry in %s", settings_file);

  logout_path = get_str_value(settings_json, "logout");
  if (logout_path == NULL) {
    logout_path = LOCAL_STDOUT_PATH;
  }

  logerr_path = get_str_value(settings_json, "logerr");
  if (logerr_path == NULL) {
    logerr_path = LOCAL_STDERR_PATH;
  }

  /* Success */
  return_value = true;

  _finish: ;
    return return_value;
}

static bool load_content_from_config_file(json_t *config_json, const char *config_file) {
  bool return_value = false;

  jid = get_str_value(config_json, "jid");
  werr2(jid == NULL, goto _finish, "There is no jid entry in %s", config_file);

  password = get_str_value(config_json, "password");
  werr2(password == NULL, goto _finish, "There is no password entry in %s", config_file);

  owner = get_str_value(config_json, "owner");
  werr2(owner == NULL, goto _finish, "There is no owner entry in %s", config_file);

  ssid = get_str_value(config_json, "ssid");
  psk  = get_str_value(config_json, "psk");
  nameserver = get_str_value(config_json, "nameserver");

  /* Set privacy based on privacy value from wyliodrin.json (if exists) */
  json_t *privacy_json = json_object_get(config_json, "privacy");
  if (privacy_json != NULL && json_is_boolean(privacy_json) && json_is_true(privacy_json)) {
    privacy = true;
  }

  const char *pong_timeout_str = get_str_value(config_json, "pong_timeout");
  if (pong_timeout_str != NULL) {
    char *pEnd;
    pong_timeout = strtol(pong_timeout_str, &pEnd, 10);
    if (pong_timeout == 0) {
      pong_timeout = DEFAULT_PONG_TIMEOUT;
    }
  }

  /* Success */
  return_value = true;

  _finish: ;
    return return_value;
}
/*************************************************************************************************/
