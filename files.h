#ifndef _FILES_H_
#define _FILES_H_

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include "xmpp.h"

#define path_ok		(strncmp (path, "/", 1 ) == 0)

#define IMPLICIT_FOLDER_PERMISSION	666
#define WRITE 	1
#define APPEND	2

#define BLOCK_DEVICE		1
#define CHARACTER_DEVICE	2
#define DIRECTORY			3
#define FIFO    			4
#define SYMLINK				8
#define REGULAR_FILE		5
#define SOCKET				6
#define UNKNOWN				7

#define FILE_E_OK				0
#define FILE_E_CREATE		1
#define FILE_E_UNKNOWN		2
#define FILE_E_WRITE		3
#define FILE_E_PATH			4
#define FILE_E_MODE			5



typedef struct  s_file
{
	char type;
	time_t atime,stime,mtime;
	long size;
	int permission;
}file;

typedef struct s_files_list
{
	char type;
	char *name;
}files_list;

void file_tag(const char *from, const char *to, int error, xmpp_stanza_t *stanza);

#endif