/**************************************************************************************************
 * MessagePack decoding
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: September 2015
 *************************************************************************************************/

#ifdef USEMSGPACK

#ifndef _USEMSGPACK_H
#define _USEMSGPACK_H



/*** DECLARATIONS ********************************************************************************/

/* Build a hashmap with keys as module names and values as handlers */
void build_modules_hashmap();

/* Handler for the w stanza */
void wmsgpack(const char *from, const char *to, const char *enc_data);

/*************************************************************************************************/



#endif /* _USEMSGPACK_H */

#endif /* USEMSGPACK */
