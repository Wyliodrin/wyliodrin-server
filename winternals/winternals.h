/**************************************************************************************************
 * Internals of Wyliodrin: typedefs, logs, errors, etc.
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _INTERNALS_H
#define _INTERNALS_H

/* Data types */
typedef signed char    int8_t;
typedef short int      int16_t;
typedef int            int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int   uint32_t;

typedef enum {
  false = 0,
  true  = 1
} bool_t;

#define LOG_FILE stdout
#define ERR_FILE stderr

#define TRUE  1
#define FALSE 0

/* Wylidrin log used when execution enters and leaves a function */
#ifdef LOG
  #define wlog(msg, ...) fprintf(LOG_FILE, "[wlog in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#else
  #define wlog(msg, ...) /* Do nothing */
#endif

/* Wylidrin err used when execution is inside a function */
#ifdef ERR
  #define werr(msg, ...) fprintf(ERR_FILE, "[werr in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);
#else
  #define werr(msg, ...) /* Do nothing */
#endif

#define wfatal(assertion, msg, ...)                                                          \
  do {                                                                                       \
    if (assertion) {                                                                         \
      fprintf(ERR_FILE, "[wfatal in %s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__);   \
      exit(EXIT_FAILURE);                                                                    \
    }                                                                                        \
  } while (0)

#define wsyserr(assertion, msg)                                     \
  do {                                                              \
    if (assertion) {                                                \
      fprintf(stderr, "[wsyserr in %s:%d] ", __FILE__, __LINE__);   \
      perror(msg);                                                  \
      exit(EXIT_FAILURE);                                           \
    }                                                               \
  } while (0)

#endif // _INTERNALS_H 
