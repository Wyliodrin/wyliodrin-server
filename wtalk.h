/**************************************************************************************************
 * WTALK INTERNALS
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com>
 * Date last modified: August 2015
 *************************************************************************************************/

#ifndef _WTALK_H
#define _WTALK_H



#define BOARDTYPE_PATH "/etc/wyliodrin/boardtype" /* File path of the file named boardtype.
                                                     This file contains the name of the board.
                                                     Example: edison or arduinogalileo. */

#define SETTINGS_PATH  "/etc/wyliodrin/settings_" /* File path of "settings_<boardtype>.json".
                                                     <boardtype> is the string found in the file
                                                     named boardtype.
                                                     Example: settings_edison.json */

#define RUNNING_PROJECTS_PATH "/etc/wyliodrin/running_projects"

#define BOARDTYPE_MAX_LENGTH     64       /* Max length of boardtype */

#define SETTINGS_PATH_MAX_LENGTH 128      /* Max length of settings file path */
#define JSON_CONTENT_MAX_LENGTH  128      /* Used in snprintf */

#define WTALK_VERSION_MAJOR "1"
#define WTALK_VERSION_MINOR "0"



#endif /* _WTALK_H */
