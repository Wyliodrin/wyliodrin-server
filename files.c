#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "files.h"

char **list_files(char *path, int *n)
{
	if(path_ok)
    {
        struct dirent **namelist;

        *n = scandir(".", &namelist, NULL, alphasort);
        if (n < 0)
            {
                perror("scandir");
                return NULL;
            }
        else 
        {
            char **files = malloc((*n) * sizeof(char *));
            int i;
            for(i=0; i<*n; i++)
            {
                files[i] = namelist[i]->d_name;
                free(namelist[i]);
            }
            free(namelist);
            return files;
        }        
    }
    else
        return NULL;	
}

file *get_file_info(char *path)
{
    if(path_ok)
    {
        struct stat sb;
        if(stat(path, &sb) < 0)
        {
            return NULL;
        }
        else
        {
            file *f = malloc(sizeof(struct s_file));
            if(f == NULL)
                return f;
            switch (sb.st_mode & S_IFMT) 
            {
                case S_IFBLK:  f->type = BLOCK_DEVICE;            break;
                case S_IFCHR:  f->type = CHARACTER_DEVICE;        break;
                case S_IFDIR:  f->type = DIRECTORY;               break;
                case S_IFIFO:  f->type = FIFO;                    break;
                case S_IFLNK:  f->type = SYMLINK;                 break;
                case S_IFREG:  f->type = REGULAR_FILE;            break;
                case S_IFSOCK: f->type = SOCKET;                  break;
                default:       f->type = UNKNOWN;                 break;
            }
            f->atime = sb.st_atime;
            f->stime = sb.st_ctime;
            f->mtime = sb.st_mtime;
            f->size = sb.st_size;
            int permission = 0;
            int file_mode = sb.st_mode;

            /*Check owner permissions*/
             if (file_mode & S_IRUSR)
                permission = permission + 400;
            if (file_mode & S_IWUSR) 
                permission = permission + 200;
            if (file_mode & S_IXUSR)
                permission = permission + 100;

            /* Check group permissions */
            if (file_mode & S_IRGRP)
                permission = permission + 40;
            if (file_mode & S_IWGRP)
                permission = permission + 20;
            if (file_mode & S_IXGRP)
                permission = permission + 10;

            /* check other user permissions */
            if (file_mode & S_IROTH)
                permission = permission + 4;
            if (file_mode & S_IWOTH)
                permission = permission + 2;
            if (file_mode & S_IXOTH)
                permission = permission + 1;

            /*check sticky bit*/
            if(file_mode & S_ISVTX)
                permission = permission + 1000;

            f->permission = permission;

            return f;
        }       
    }
    return NULL;
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
    int n;
    //char ** f = list_files(pwd, &n);
    int i;
    // for(i=0; i<n; i++)
    // {
    //     printf("f=%s\n",f[i]);
    // }
    file *f = get_file_info("home/pi/iiii");
    if(f != NULL)
    {
        printf("type = %d\n",f->type);
        printf("size = %d\n", f->size);
        printf("modif = %d\n", f->atime);
        printf("perm = %d\n", f->permission);
    }
	return 0;
}