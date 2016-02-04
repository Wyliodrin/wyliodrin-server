/**************************************************************************************************
 * Paths used by wtalk
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: October 2015
 *************************************************************************************************/

#ifndef _WTALK_H
#define _WTALK_H



/*** DEFINES *************************************************************************************/

#define BOARDTYPE_PATH "/etc/wyliodrin/boardtype" /* Path of the file named boardtype.
                                                   * This file contains the name of the board.
                                                   * Example: edison or arduinogalileo.
                                                   */

#define SETTINGS_PATH  "/etc/wyliodrin/settings_" /* Path of "settings_<boardtype>.json".
                                                   * <boardtype> is the string found in the file
                                                   * named boardtype.
                                                   * Example: settings_edison.json
                                                   * This is the settings configuration file.
                                                   */

#define RUNNING_PROJECTS_PATH "/wyliodrin/running_projects" /* This file contains the ids of the
                                                               running projects. In case the board
                                                               shuts down unexpectedly, the running
                                                               projects will be automatically
                                                               started at reboot. */

#define RPI_WIFI_PATH "/etc/wyliodrin/wireless.conf" /* Wifi connection credentials used by
                                                        Raspberry Pi */

#define SETTINGS_PATH_MAX_LENGTH 128      /* Max length of settings file path */
#define JSON_CONTENT_MAX_LENGTH  128      /* Used in snprintf */

#define LOCAL_STDOUT_PATH "/etc/wyliodrin/hlogs.out"
#define LOCAL_STDERR_PATH "/etc/wyliodrin/hlogs.err"

#define DEFAULT_PONG_TIMEOUT 15 /* Time in seconds between pongs */

/*************************************************************************************************/



#endif /* _WTALK_H */
