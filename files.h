#ifndef _FILES_H_
#define _FILES_H_

#include <dirent.h>

#define path_ok		(strncmp (path, "/", 1 ) == 0)

#define IMPLICIT_FOLDER_PERMISSION	666
#define WRITE 	1
#define APPEND	2


typedef struct  file
{
	int n;
	struct dirent **namelist;
};
char ** list_files(char *path);
unsigned char get_file_type(char *path);
void remove_file(char *path);
void create_folder(char *path, int permission);
void create_file(char *path);
void write_to_file(char *path, char *buf, int mode);

#endif