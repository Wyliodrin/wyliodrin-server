/**************************************************************************************************
 * Redis connection API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/

#ifndef _WREDIS_H
#define _WREDIS_H



/*** API *****************************************************************************************/

/**
 * Redis initialization
 */
void init_redis();

/**
 * Publish to redis
 */
void publish(const char *str, int data_len);

/*************************************************************************************************/



#endif /* _WREDIS_H */
