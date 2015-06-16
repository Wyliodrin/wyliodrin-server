/**************************************************************************************************
 * Working with JSONs
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _WJSON_H
#define _WJSON_H

#define BUFFER_SIZE (1 * 1024) /* 1 KB */

#define SETTINGS_PATH  "/etc/settings/settings_" /* Settings file path */

/**
 * Open <filename>, read the JSON object, convert it to json_t and return it.
 *
 * PARAMETERS:
 *    filename - filename containt the JSON object
 *
 * RETURN:
 *    pointer to converted json_t or NULL in case of errors  
 */
json_t* file_to_json_t(const char *filename);

#endif // _WJSON_H
