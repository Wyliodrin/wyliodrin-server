/**************************************************************************************************
 * Paths used by wtalk
 *
 * Author: Ioana CULIC <ioana.culic@wyliodrin.com>
 *************************************************************************************************/
#ifndef _WYAPP_H
#define _WYAPP_H



/*** DEFINES *************************************************************************************/

#define BOARDTYPE_PATH "/etc/wyliodrin/boardtype" /* Contains the name of the board */

#define SETTINGS_PATH  "/etc/wyliodrin/settings_" /* Path of "settings_<boardtype>.json".
                                                   * <boardtype> is the string found in boardtype.
                                                   */

#define RPI_WIFI_PATH "/etc/wyliodrin/wireless.conf" /* Wifi connection credentials used by
                                                      * Raspberry Pi */

#define LOCAL_STDOUT_PATH "/etc/wyliodrin/logs.out"
#define LOCAL_STDERR_PATH "/etc/wyliodrin/logs.err"

#define DEFAULT_PONG_TIMEOUT 15 /* Time in seconds between pongs */

/*************************************************************************************************/

#endif /* _WYAPP_H */