/**************************************************************************************************
 * Shells module API
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifdef SHELLS

#ifndef _SHELLS_H
#define _SHELLS_H



/*** INCLUDES ************************************************************************************/

#include <strophe.h>        /* xmpp library */
#include "../../libds/ds.h" /* hashmap      */

/*************************************************************************************************/



/*** DEFINES *************************************************************************************/

#define MAX_SHELLS 256 /* Maximum number of shells */

/*************************************************************************************************/



/*** TYPEDEFS ************************************************************************************/

typedef struct {
  long int width;      /* width */
  long int height;     /* height */
  int pid;             /* PID */
  int fdm;             /* PTY file descriptor */
  int id;              /* shell id */
  char *request;       /* open request */
  char *projectid;     /* projectid in case of make shell */
  char *userid;        /* userid in case of make shell */
  bool is_connected;   /* is project connected */
} shell_t;

/*************************************************************************************************/



/*** API *****************************************************************************************/

/**
 * Initialize with NULL shells_vector
 */
void init_shells();

/**
 * Parse shells commands
*/
void shells(hashmap_p hm);

/**
 * Start projects that died on board shutdown.
 */
// void start_dead_projects();

/*************************************************************************************************/



#endif /* _SHELLS_H */

#endif /* SHELLS */
