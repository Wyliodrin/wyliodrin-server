/**************************************************************************************************
 * Shells helper
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: April 2015
 *************************************************************************************************/

#ifndef _SHELLS_HELPER_H
#define _SHELLS_HELPER_H

/**
 * Read routine.
 *
 * Read data from PTY where screen has been opened and send it to server via shell keys.
 *
 * PARAMETERS:
 *    args - shell_t
 */
void *read_thread(void *args);


#endif /* _SHELLS_HELPER_H */