/**************************************************************************************************
 * JSON handling implementation
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/



/*** INCLUDES ************************************************************************************/

#include <fcntl.h>   /* open            */
#include <jansson.h> /* json_t handling */
#include <unistd.h>  /* read, close     */

#include "../winternals/winternals.h" /* logs and errs */

#include "wjson.h" /* API */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define BUFFER_SIZE (1 * 1024) /* Maximum buffer size used to store the content from a file which
                                * store a json.
                                */

/*************************************************************************************************/



/*** API IMPLEMENTATION **************************************************************************/

json_t* file_to_json(const char *filename) {
	json_t *return_value = NULL;
  char *buffer = NULL;

	/* Open filename */
	int filename_fd = open(filename, O_RDONLY);
	wsyserr2(filename_fd == -1, goto _finish, "Could not open %s", filename);

	/* Allocate memory for buffer */
	buffer = calloc(BUFFER_SIZE, sizeof(char));
	wsyserr2(buffer == NULL, goto _finish, "Could not allocate memory for buffer");

	/* Read data in buffer */
	int read_rc = read(filename_fd, buffer, BUFFER_SIZE - 1);
	wsyserr2(read_rc == -1, goto _finish, "Could not read from %s", filename);
  werr2(read_rc == BUFFER_SIZE -1, goto _finish, "Json too long in %s", filename);

	/* Convert data in json */
	json_error_t error_json;
	json_t *json = json_loads(buffer, 0, &error_json);
	werr2(json == NULL, goto _finish, "Invalid json in %s: %s", filename, error_json.text);

	/* Success */
	return_value = json;

  _finish: ;
  	if (filename_fd != -1) {
	  	int close_rc = close(filename_fd);
	    wsyserr2(close_rc == -1, return_value = NULL, "Could not close %s", filename);
  	}

  	if (buffer != NULL) {
  		free(buffer);
  	}

		return return_value;
}


const char *get_str_value(json_t *json, char *key) {
  json_t *value_json = json_object_get(json, key);

  if (value_json != NULL && json_is_string(value_json) &&
      strlen(json_string_value(value_json)) > 0) {
    return json_string_value(value_json);
  }

  return NULL;
}

/*************************************************************************************************/
