/**************************************************************************************************
 * Internals of WTalk: logs, errors, bools.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: June 2015
 *************************************************************************************************/

#ifndef _INTERNALS_H
#define _INTERNALS_H



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "../logs/logs.h"



extern bool privacy;



#define LOG_FILE stdout
#define ERR_FILE stderr


#define FALSE 0
#define TRUE  1


#ifdef LOG
  #define wlog(msg, ...) fprintf(LOG_FILE, "[wlog in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#else
  #define wlog(msg, ...) /* Do nothing */
#endif


#ifdef ERR
  #define werr(msg, ...)                                                                   \
    do {                                                                                   \
      fprintf(ERR_FILE, "[werr in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
      if (!privacy) {                                                                      \
        add_log(msg, ##__VA_ARGS__);                                                       \
      }                                                                                    \
    } while (0)
#else
  #define werr(msg, ...) /* Do nothing */
#endif


#ifdef ERR
  #define werr2(assertion, msg, ...)                                                       \
    do {                                                                                   \
      if (assertion) {                                                                     \
        fprintf(ERR_FILE, "[werr in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
      }                                                                                    \
  } while (0)
#else
  #define werr2(assertion, msg, ...) /* Do nothing */
#endif


#define wfatal(assertion, msg, ...)                                                        \
  do {                                                                                     \
    if (assertion) {                                                                       \
      fprintf(ERR_FILE, "[wfatal in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
      exit(EXIT_FAILURE);                                                                  \
    }                                                                                      \
  } while (0)


#define wsyserr(assertion, msg)                                     \
  do {                                                              \
    if (assertion) {                                                \
      fprintf(ERR_FILE, "[wsyserr in %s:%d] ", __FILE__, __LINE__); \
      perror(msg);                                                  \
      exit(EXIT_FAILURE);                                           \
    }                                                               \
  } while (0)



#endif /* _INTERNALS_H */
