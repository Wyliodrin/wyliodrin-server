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

#define FILE_OK				0
#define FILE_E_CREATE		1
#define FILE_E_UNKNOWN		2
#define FILE_E_WRITE		3
#define FILE_E_PATH			4



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
files_list** list_files(char *path, int *n);
file *get_file_info(char *path);
int remove_file(char *path);
int create_folder(char *path, int permission);
int create_file(char *path);
int write_to_file(char *path, char *buf, int mode);

#endif