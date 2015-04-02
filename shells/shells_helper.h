/**************************************************************************************************
 * Shells helper
 *************************************************************************************************/

#ifndef _SHELLS_HELPER_H
#define _SHELLS_HELPER_H

/**
 * Open new pty
 */
int openpty(int *fdm, int *fds);

/**
 * Read routine
 */
void *read_thread(void *args);


#endif /* _SHELLS_HELPER_H */