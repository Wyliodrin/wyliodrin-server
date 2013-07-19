#ifndef _FILES_H_
#define _FILES_H_

#include <dirent.h>

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


typedef struct  s_file
{
	int type;
	time_t atime,stime,mtime;
	long size;
	int permission;
}file;
char **list_files(char *path, int *n);
file *get_file_info(char *path);
void remove_file(char *path);
void create_folder(char *path, int permission);
void create_file(char *path);
void write_to_file(char *path, char *buf, int mode);

#endif