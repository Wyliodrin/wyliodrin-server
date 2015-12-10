/**************************************************************************************************
 * Paths used by wtalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: November 2015
 *************************************************************************************************/

#ifndef _WTALK_H
#define _WTALK_H



/*** DEFINES *************************************************************************************/

#define BOARDTYPE_PATH "/etc/wyliodrin/boardtype" /* Contains the name of the board */

#define SETTINGS_PATH  "/etc/wyliodrin/settings_" /* Path of "settings_<boardtype>.json".
                                                   * <boardtype> is the string found in boardtype.
                                                   */

#define RUNNING_PROJECTS_PATH "/wyliodrin/running_projects" /* This file contains the ids of the
                                                             * running projects. In case the board
                                                             * shuts down unexpectedly, the running
                                                             * projects will be automatically
                                                             * started at reboot. */

#define RPI_WIFI_PATH "/etc/wyliodrin/wireless.conf" /* Wifi connection credentials used by
                                                      * Raspberry Pi */

#define LOCAL_STDOUT_PATH "/etc/wyliodrin/logs.out"
#define LOCAL_STDERR_PATH "/etc/wyliodrin/logs.err"

#define DEFAULT_PONG_TIMEOUT 15 /* Time in seconds between pongs */

/*************************************************************************************************/



#endif /* _WTALK_H */
