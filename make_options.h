#ifndef _MAKE_OPTIONS_H_
#define _MAKE_OPTIONS_H_

#define MAKE_E_PATH		1
#define MAKE_E_FORK		2

#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <dirent.h>
#include <stdlib.h>
#include <time.h>

#include "xmpp.h"

void make_tag(const char *from, const char *to, int error, xmpp_stanza_t *stanza);


#endif