/**************************************************************************************************
 * JSON handling
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#ifndef _WJSON_H
#define _WJSON_H

#include <jansson.h> /* json handling */

/**
 * Open <filename>, read the JSON object, convert it to json_t and return it.
 *
 * PARAMETERS:
 *    filename - filename that contains a JSON object
 *
 * RETURN:
 *    pointer to converted json_t or NULL if filename does not exist or
 *    it's content is not a JSON object
 */
json_t* file_to_json_t(const char *filename);

#endif /* _WJSON_H */
