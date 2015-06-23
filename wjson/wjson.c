/**************************************************************************************************
 * Working with JSONs
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: June 2015
 *************************************************************************************************/

#include <fcntl.h>   /* open            */
#include <unistd.h>  /* read, close     */
#include <jansson.h> /* json_t handling */

#include "wjson.h"                    /* declarations  */
#include "../winternals/winternals.h" /* logs and errs */


#define BUFFER_SIZE (1 * 1024) /* Maximum size for <filename> */


json_t* file_to_json_t(const char *filename) {
	json_t *ret = NULL; /* return value       */
	json_error_t error; /* Error information  */
	int rc_int; /* Return code od interger type */

	/* Open file containing the JSON */
	int filename_fd = open(filename, O_RDONLY);
	if (filename_fd < 0) {
		werr("open %s", filename);
		perror("open");
		goto _exit;
	}

	/* Allocate memory for buffer */
	char *buffer = (char*) calloc(BUFFER_SIZE, 1); /* buffer used in read */
	wsyserr(buffer == NULL, "calloc");

	/* Read JSON buffer */
	rc_int = read(filename_fd, buffer, BUFFER_SIZE);
	wsyserr(rc_int < 0, "read");

	/* Convert JSON buffer */
	ret = json_loads(buffer, 0, &error);
	if(ret == NULL) {
		werr("Undecodable JSON in %s\n", filename);
		goto _cleaning;
	}

	/* Check whether ret is object */
	if(!json_is_object(ret)) {
		werr("JSON in %s\n is not an object", filename);
		json_decref(ret);
		ret = NULL;
	}

_cleaning:
	free(buffer);
	close(filename_fd);

_exit:
	return ret;
}
