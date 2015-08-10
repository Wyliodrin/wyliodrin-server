/**************************************************************************************************
 * JSON handling
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: July 2015
 *************************************************************************************************/

#include <fcntl.h>   /* open            */
#include <errno.h>   /* errno           */
#include <string.h>  /* strerror        */
#include <unistd.h>  /* read, close     */
#include <jansson.h> /* json_t handling */

#include "../winternals/winternals.h" /* logs and errs */
#include "wjson.h"



#define BUFFER_SIZE (1 * 1024) /* Maximum size for <filename> */



json_t* file_to_json_t(const char *filename) {
	/* Open file containing the JSON */
	int filename_fd = open(filename, O_RDONLY);
	if (filename_fd < 0) {
		werr("Failed to open %s because %s", filename, strerror(errno));
		return NULL;
	}

	/* Allocate memory for buffer */
	char *buffer = calloc(BUFFER_SIZE, sizeof(char)); /* buffer used in read */
	wsyserr(buffer == NULL, "calloc");

	/* Read JSON buffer */
	int rc_int = read(filename_fd, buffer, BUFFER_SIZE); /* return code of integer type */
	close(filename_fd);
	wsyserr(rc_int < 0, "read");

	/* Convert JSON buffer */
	json_error_t error_json; /* json error information */
	json_t * ret = json_loads(buffer, 0, &error_json); /* return value */
	free(buffer);
	if (ret == NULL) {
		werr("Undecodable JSON in %s\n", filename);
		return NULL;
	}

	/* Check whether ret is object */
	if (!json_is_object(ret)) {
		werr("JSON in %s\n is not an object", filename);
		json_decref(ret);
		return NULL;
	}

	return ret;
}

const char *get_str_value(json_t *json, char *key) {
  json_t *value_json = json_object_get(json, key); /* value as JSON value */

  /* Sanity checks */
  if (value_json == NULL || !json_is_string(value_json) || strlen(json_string_value(value_json)) == 0) {
    return NULL;
  } else {
    return json_string_value(value_json);
  }
}