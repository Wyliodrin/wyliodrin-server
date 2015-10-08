/**************************************************************************************************
 * Send logs in cloud implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <curl/curl.h> /* send via https */
#include <pthread.h>   /* threads        */
#include <stdbool.h>   /* bool           */
#include <stdio.h>     /* fprintf        */
#include <stdlib.h>    /* calloc         */
#include <string.h>    /* strcat         */
#include <time.h>      /* time           */
#include <unistd.h>    /* sleep          */

#include "logs.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define MAX_LOG_SIZE 512
#define MAX_LOGS     1024
#define UPDATE_TIME  1    /* Time in second until next logs send */

/*************************************************************************************************/



/*** STATIC VARIABLES ****************************************************************************/

static pthread_mutex_t logs_mutex = PTHREAD_MUTEX_INITIALIZER;
static char **logs;
static unsigned int logs_size = 0;
static bool is_send_logs_routine_started = false;

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern const char *jid;

/*************************************************************************************************/



/*** STATIC FUNCTIONS DECLARATIONS ***************************************************************/

/**
 * Routine that sends regularly logs to cloud.
 */
static void *send_logs_routine(void *args);

/**
 * Start the send logs routine.
 */
static void start_send_logs_routine();

/*************************************************************************************************/



/*** API IMPLEMENTATION **************************************************************************/

void add_log(log_type_t log_type, const char *msg, ...) {
  if (logs_size == MAX_LOGS) {
    fprintf(stderr, "Maximum number of logs reached\n");
    return;
  }

  if (!is_send_logs_routine_started) {
    start_send_logs_routine();
  }

  /* Get time */
  time_t rawtime;
  struct tm *timeinfo;
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  if (timeinfo == NULL) {
    fprintf(stderr, "Could not get time\n");
    return;
  }
  char *time_str = asctime(timeinfo);

  /* Build full msg */
  va_list aptr;
  char *full_msg = malloc((MAX_LOG_SIZE - strlen(time_str) - 16) * sizeof(char));
  va_start(aptr, msg);
  vsnprintf(full_msg, MAX_LOG_SIZE, msg, aptr);
  va_end(aptr);

  /* Build final msg */
  char *str = malloc(MAX_LOG_SIZE * sizeof(char));
  time_str[strlen(time_str) - 1] = 0; /* Get rid of the newline character */
  if (log_type == ERROR_LOG) {
    snprintf(str, MAX_LOG_SIZE, "[ERROR: %s] %s", time_str, full_msg);
  } else if (log_type == INFO_LOG) {
    snprintf(str, MAX_LOG_SIZE, "[INFO: %s] %s", time_str, full_msg);
  } else if (log_type == SYSERROR_LOG) {
    snprintf(str, MAX_LOG_SIZE, "[SYSERROR: %s] %s", time_str, full_msg);
  } else {
    fprintf(stderr, "Invalid log type\n");
    free(full_msg);
    free(str);
    return;
  }
  free(full_msg);

  pthread_mutex_lock(&logs_mutex);
  logs[logs_size] = str;
  logs_size++;
  pthread_mutex_unlock(&logs_mutex);
}

/*************************************************************************************************/



/*** STATIC FUNCTIONS IMPLEMENTATIONS ************************************************************/

static void *send_logs_routine(void *args) {
  char URL[512];

  while (1) {
    sleep(UPDATE_TIME);

    /* Send logs */
    if (logs_size != 0) {
      pthread_mutex_lock(&logs_mutex);

      char *big_msg = calloc(logs_size * MAX_LOG_SIZE, sizeof(char));
      if (big_msg == NULL) {
        fprintf(stderr, "Calloc failed\n");
        continue;
      }
      sprintf(big_msg, "{\"str\":\"");
      int i;
      for (i = 0; i < logs_size; i++) {
        strcat(big_msg, logs[i]);
        free(logs[i]);
        if (i != logs_size - 1) {
          strcat(big_msg, "\\n");
        }
      }
      strcat(big_msg, "\"}");

      logs_size = 0;

      pthread_mutex_unlock(&logs_mutex);

      CURL *curl = curl_easy_init();
      if (curl == NULL) {
        fprintf(stderr, "Initialization on curl failed\n");
        continue;
      }

      char *domain = strrchr(jid, '@');
      if (domain == NULL) {
        fprintf(stderr, "Jid does not contain an at\n");
        continue;
      }

      domain++;

      int snprintf_rc = snprintf(URL, 512, "https://%s/gadgets/logs/%s", domain, jid);
      if (snprintf_rc < 0) {
        fprintf(stderr, "Could not build URL for sending logs\n");
        continue;
      }
      if (snprintf_rc >= 512) {
        fprintf(stderr, "URL too long\n");
        continue;
      }

      curl_easy_setopt(curl, CURLOPT_URL, URL);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 50L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (const char*)big_msg);
      struct curl_slist *list = NULL;
      list = curl_slist_append(list, "Content-Type: application/json");
      list = curl_slist_append(list, "Connection: close");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
      #ifdef VERBOSE
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      #endif /* VERBOSE */

      CURLcode res = curl_easy_perform(curl);

      if (res != CURLE_OK) {
        fprintf(stderr, "Curl failed");
      }

      curl_easy_cleanup(curl);
    }
  }

  fprintf(stderr, "Routine for sending logs should never stop\n");

  return NULL;
}


static void start_send_logs_routine() {
  is_send_logs_routine_started = true;

  logs = malloc(MAX_LOGS * sizeof(char *));
  if (logs == NULL) {
    is_send_logs_routine_started = false;
    fprintf(stderr, "Could not allocate memory for logs\n");
    return;
  }

  pthread_t send_logs_thread;
  int rc = pthread_create(&send_logs_thread, NULL, send_logs_routine, NULL);
  if (rc != 0) {
    is_send_logs_routine_started = false;
    fprintf(stderr, "Could not create thread for sending logs\n");
    return;
  }

  pthread_detach(send_logs_thread);
}

/*************************************************************************************************/
