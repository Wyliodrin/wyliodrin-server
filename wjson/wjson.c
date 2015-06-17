/**************************************************************************************************
 * Working with JSONs
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#include <fcntl.h>   /* open */
#include <unistd.h>  /* read, close */
#include <jansson.h> /* json_t stuff */

#include "wjson.h"                    /* JSON stuff */
#include "../winternals/winternals.h" /* logs and errs */

json_t* file_to_json_t(const char *filename) {
	wlog("file_to_json_t(%s)", filename);

	char *buffer;       /* JSON text */
	json_t *root;       /* Main json_t object */ 
	json_error_t error; /* Error information */
	int fd;             /* File descriptor with JSON text */
	int ret_read;       /* Read return value */

	/* Allocate memory for buffer */
	buffer = (char*) calloc(BUFFER_SIZE, 1);
	if(buffer == NULL) {
		perror("[perror] Allocate memory for buffer");

		wlog("Return NULL due to error in memory allocation");
		return NULL;
	}

	/* Open file with with JSON text */
	fd = open(filename, O_RDONLY);
	if(fd < 0) {
		perror("[perror] Open file with JSON text");
		werr ("Error open %s", filename);
		free(buffer);

		wlog("Return NULL due to trying to open %s", filename);
		return NULL;
	}

	/* Read JSON buffer */
	ret_read = read(fd, buffer, BUFFER_SIZE);
	if(ret_read < 0) {
		perror("[perror] Read JSON buffer");
		free(buffer);
		close(fd);

		wlog("Return NULL due to reading error from %s", filename);
		return NULL;
	}

	/* Convert JSON buffer */
	root = json_loads(buffer, 0, &error);
	if(root == NULL) {
		werr("Undecodable JSON in %s\n", filename);
		free(buffer);
		close(fd);

		wlog("Return NULL due to undecodable JSON");
		return NULL;
	}

	/* Check if root is object */
	if(!json_is_object(root)) {
		werr("JSON in %s\n is not an object", filename);
		free(buffer);
		close(fd);

		wlog("Return NULL because JSON in %s is not object", filename);
		return NULL;	
	}

	/* Cleaning */
	free(buffer);
	close(fd);

	wlog("Return successfully converted JSON object");
	return root;
}
