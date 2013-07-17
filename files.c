#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "files.h"

struct files *get_files(char *path)
{
	if(path_ok)
    {
        struct dirent **namelist;
        int n;

        n = scandir(".", &namelist, NULL, alphasort);
        if (n < 0)
            {
                perror("scandir");
                return NULL;
            }
        else 
        {
            struct file * f = malloc(sizeof(struct file));
            f->namelist = namelist;
            f->n = n;
           return f;
        }
        
    }
    else
        return NULL;	
}

char ** list_files(char *path)
{
    struct file * f= get_files(path);
    if(f == NULL)
        return NULL;
    struct dirent **namelist = f->namelist; 
    int n = f->n;
    char **files_name = malloc(n * sizeof(char *));
    int i;
    for (i=0; i<n; i++)
    {
        files_name[i] = namelist[i]->d_name;
    }
    return  files_name;
}

unsigned char get_file_type(char *path)
{
    struct file * f= get_files(path);
    if(f != NULL)
    {
       return (f->namelist)[0]->d_type;
    }
    else return 0;
}

void create_folder(char *path, int permission)
{
    struct stat st = {0};
    if(path_ok)
    {
        if (stat(path, &st) == -1)
            if(permission != 0)
            mkdir(path, permission);
        else
            mkdir(path,IMPLICIT_FOLDER_PERMISSION);
    }
}

void remove_file(char *path)
{
    if(path_ok)
        remove(path);
}

void create_file(char *path)
{
    if(path_ok)
    {
        FILE *f = fopen(path, "w");
        fclose(f);    
    }    
}

void write_to_file(char *path, char *buf, int mode)
{
    FILE *f;
    if(path_ok)
    {
        if(mode == APPEND)
            f = fopen(path, "a");
        else
            f = fopen(path, "w");
        if(f != NULL)
            fprintf(f,"%s",buf);
        fclose(f); 
    }
}


int main()
{
	char *pwd = get_current_dir_name();
    //printf("ls = %c\n",list_files(pwd)[5]);
   printf("blah = %d\n",get_file_type(pwd));
    //printf("bla\n");
    //create_folder("/home/pi/test",0);
    remove_file("/home/pi/test");
    create_file("/home/pi/iiii");
    write_to_file("/home/pi/iiii", "ana", WRITE);
    write_to_file("/home/pi/iiii", "are", APPEND);
	return 0;
}