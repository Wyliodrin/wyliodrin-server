/**************************************************************************************************
 * JSON handling API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifndef _WJSON_H
#define _WJSON_H



/*** INCLUDES ************************************************************************************/

#include <jansson.h>

/*************************************************************************************************/



/*** API *****************************************************************************************/

/**
 * Load json content in json_t from filename. Returns NULL in case of error.
 */
json_t* file_to_json(const char *filename);

/**
 * Get the string value of the key <key> in the json object <json>.
 *
 * Returns the associated value of string as a null terminated UTF-8 encoded string,
 * or NULL if there is not a key <key> in <json>, or there is a key <key>, but not of
 * string type.
 *
 * The retuned value is read-only and must not be modified or freed by the user.
 * It is valid as long as string exists, i.e. as long as its reference count has not dropped to 0.
 */
const char *get_str_value(json_t *json, char *key);

/*************************************************************************************************/



#endif /* _WJSON_H */
