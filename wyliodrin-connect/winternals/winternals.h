/**************************************************************************************************
 * Internals of wtalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifndef _WINTERNALS_H
#define _WINTERNALS_H



/*** INCLUDES ************************************************************************************/

#include <errno.h>    /* errno    */
#include <stdbool.h>  /* bool     */
#include <stdio.h>    /* fprintf  */
#include <string.h>   /* strerror */

#include "../logs/logs.h" /* add_log */

/*************************************************************************************************/



/*** EXTERN VARIABLES ****************************************************************************/

extern bool privacy; /* Don't add logs if privacy is set to true */

/*************************************************************************************************/



/*** MACROS **************************************************************************************/

#ifdef LOG
  #define wlog(msg, ...)                                                                          \
    do {                                                                                          \
      fprintf(stdout, "[wlog in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);            \
    } while (0)
#else
  #define wlog(msg, ...) /* Do nothing */
#endif


#define werr(msg, ...)                                                                            \
  do {                                                                                            \
    fprintf(stderr, "[werr in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);              \
    if (!privacy) {                                                                               \
      add_log(ERROR_LOG, msg, ##__VA_ARGS__);                                                     \
    }                                                                                             \
  } while (0)


#define werr2(assertion, action, msg, ...)                                                        \
  do {                                                                                            \
    if (assertion) {                                                                              \
      fprintf(stderr, "[werr in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);            \
      if (!privacy) {                                                                             \
        add_log(ERROR_LOG, msg, ##__VA_ARGS__);                                                   \
      }                                                                                           \
      action;                                                                                     \
    }                                                                                             \
  } while (0)


#define winfo(msg, ...)                                                                           \
  do {                                                                                            \
    fprintf(stderr, "[winfo in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);             \
    if (!privacy) {                                                                               \
      add_log(INFO_LOG, msg, ##__VA_ARGS__);                                                      \
    }                                                                                             \
  } while (0)


#define wsyserr(assertion, msg, ...)                                                              \
  do {                                                                                            \
    if (assertion) {                                                                              \
      fprintf(stderr, "[syserr in %s:%d] " msg ": %s\n", __FILE__, __LINE__, ##__VA_ARGS__,       \
                                                         strerror(errno));                        \
      if (!privacy) {                                                                             \
        add_log(SYSERROR_LOG, msg, ##__VA_ARGS__);                                                \
      }                                                                                           \
    }                                                                                             \
  } while (0)


#define wsyserr2(assertion, action, msg, ...)                                                     \
  do {                                                                                            \
    if (assertion) {                                                                              \
      fprintf(stderr, "[syserr in %s:%d] " msg ": %s\n", __FILE__, __LINE__, ##__VA_ARGS__,       \
                                                         strerror(errno));                        \
      if (!privacy) {                                                                             \
        add_log(SYSERROR_LOG, msg, ##__VA_ARGS__);                                                \
      }                                                                                           \
      action;                                                                                     \
    }                                                                                             \
  } while (0)

/*************************************************************************************************/



#endif /* _WINTERNALS_H */
