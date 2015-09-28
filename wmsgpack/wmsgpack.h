/**************************************************************************************************
 * MessagePack decoding
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef USEMSGPACK

#ifndef _USEMSGPACK_H
#define _USEMSGPACK_H



/*** INCLUDES ************************************************************************************/

#include <stdarg.h>

/*************************************************************************************************/



/*** MACROS **************************************************************************************/

#define build_msgpack_map(size_addr, ...)                                                         \
        _build_msgpack_map(size_addr, __FILE__, __LINE__,                                         \
                           sizeof((char* []){__VA_ARGS__})/sizeof(char *), ##__VA_ARGS__)

/*************************************************************************************************/



/*** DECLARATIONS ********************************************************************************/

/* Build a hashmap with keys as module names and values as handlers */
void build_modules_hashmap();

/* Build msgpack map */
char *_build_msgpack_map(int *size_addr, char *file, int line,
                         int num_params, ...);

/* Handler for the w stanza */
void wmsgpack(const char *from, const char *to, const char *enc_data);

/*************************************************************************************************/



#endif /* _USEMSGPACK_H */

#endif /* USEMSGPACK */
