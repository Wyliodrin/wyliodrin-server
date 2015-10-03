/**************************************************************************************************
 * Internals of WTalk: logs, errors, bools.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <curl/curl.h>

#include "../winternals/winternals.h"
#include "logs.h"

#define MAX_LOG_SIZE 512
#define MAX_LOGS     1024

static pthread_mutex_t logs_mutex = PTHREAD_MUTEX_INITIALIZER;
static char **logs;
static unsigned int logs_size = 0;
static bool are_logs_sent = false;

extern const char *jid_str;

static void *send_logs_routine(void *args) {
  int rc_int;
  int i;
  char *big_msg;
  CURL *curl;
  CURLcode res;
  char URL[256];

  while (1) {
    /* Send logs */
    if (logs_size != 0) {
      rc_int = pthread_mutex_lock(&logs_mutex);
      wsyserr(rc_int != 0, "pthread_mutex_lock");

      big_msg = calloc(logs_size * MAX_LOG_SIZE, sizeof(char));
      sprintf(big_msg, "{\"str\":\"");
      for (i = 0; i < logs_size; i++) {
        strcat(big_msg, logs[i]);
        free(logs[i]);
        if (i != logs_size - 1) {
          strcat(big_msg, "\\n");
        }
      }
      strcat(big_msg, "\"}");

      logs_size = 0;

      curl = curl_easy_init();
      if (curl == NULL) {
        fprintf(stderr, "CURL failed\n");
        return NULL;
      }

      char *domain = strrchr(jid_str, '@');
      if (domain == NULL) {
        werr("jid does not contain an at: %s", jid_str);
        char static_domain[] = "wyliodrin.com";
        domain = static_domain;
      } else {
        domain++;
      }

      snprintf(URL, 255, "https://%s/gadgets/logs/%s", domain, jid_str);
      curl_easy_setopt(curl, CURLOPT_URL, URL);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, 50L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, (const char*)big_msg);
      struct curl_slist *list = NULL;
      list = curl_slist_append(list, "Content-Type: application/json");
      list = curl_slist_append(list, "Connection: close");
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
      #ifdef LOGS
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
      #endif

      res = curl_easy_perform(curl);

      /* Check for errors */
      if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed");
      }

      /* always cleanup */
      curl_easy_cleanup(curl);

      rc_int = pthread_mutex_unlock(&logs_mutex);
      wsyserr(rc_int != 0, "pthread_mutex_lock");
    }

    sleep(1);
  }

  return NULL;
}

static void send_logs() {
  are_logs_sent = true;

  logs = malloc(MAX_LOGS * sizeof(char *));

  pthread_t t;
  int rc;

  rc = pthread_create(&t, NULL, send_logs_routine, NULL);
  wsyserr(rc < 0, "pthread_create");
  pthread_detach(t);
}

void add_log(int log_type, const char *msg, ...) {
  /* Don't add any more logs in case of full buffer */
  if (logs_size == MAX_LOGS) {
    fprintf(stderr, "Maximum number of logs\n");
    return;
  }

  if (!are_logs_sent) {
    send_logs();
  }

  int rc_int;
  time_t rawtime;
  struct tm * timeinfo;
  va_list aptr;

  /* Get time */
  time(&rawtime);
  timeinfo = localtime(&rawtime);

  /* Build full msg*/
  char *full_msg = malloc((MAX_LOG_SIZE - strlen(asctime(timeinfo)) - 5) * sizeof(char));
  va_start(aptr, msg);
  vsprintf(full_msg, msg, aptr);
  va_end(aptr);

  /* Build to send error message */
  char *str = malloc(MAX_LOG_SIZE * sizeof(char));
  char *p = asctime(timeinfo);
  p[strlen(p) - 1] = '\0'; /* Get rid of the newline character */
  if (log_type == ERR_MSG) {
    snprintf(str, MAX_LOG_SIZE - 1, "[ERR  : %s] %s", p, full_msg);
  } else if (log_type == INFO_MSG) {
    snprintf(str, MAX_LOG_SIZE - 1, "[INFO : %s] %s", p, full_msg);
  } else {
    snprintf(str, MAX_LOG_SIZE - 1, "[LOG  : %s] %s", p, full_msg);
  }
  free(full_msg);


  rc_int = pthread_mutex_lock(&logs_mutex);
  wsyserr(rc_int != 0, "pthread_mutex_lock");

  logs[logs_size] = str;
  logs_size++;

  rc_int = pthread_mutex_unlock(&logs_mutex);
  wsyserr(rc_int != 0, "pthread_mutex_lock");
}
