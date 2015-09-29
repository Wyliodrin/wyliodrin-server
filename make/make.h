/**************************************************************************************************
 * Make module
 *
 * Author: Razvan Madalin MATEI <matei.rm94@gmail.com
 * Date last modified: May 2015
 *************************************************************************************************/

#ifdef MAKE

#ifndef _MAKE_H
#define _MAKE_H

#include "../libds/ds.h"

void init_make();

void make(const char *from, const char *to, hashmap_p h);

#endif /* _MAKE_H */

#endif /* MAKE */
