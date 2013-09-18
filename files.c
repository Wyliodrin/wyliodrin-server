#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "files.h"


files_list **list_files(char *path, int *n)
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
            files_list *files = malloc((*n) * sizeof(files_list));
            int i;
            for(i=0; i<*n; i++)
            {
                files[i]->name = namelist[i]->d_name;
                switch (namelist[i]->d_type)
                {
                    case DT_FIFO:   files[i]->type = FIFO;                  break;
                    case DT_CHR :   files[i]->type = CHARACTER_DEVICE;      break;
                    case DT_DIR :   files[i]->type = DIRECTORY;             break;
                    case DT_BLK :   files[i]->type = BLOCK_DEVICE;          break;
                    case DT_REG :   files[i]->type = REGULAR_FILE;          break;
                    case DT_LNK :   files[i]->type = SYMLINK;               break;
                    case DT_SOCK:   files[i]->type = SOCKET;                break;
                    default     :   files[i]->type = UNKNOWN;               break;
                }

                free(namelist[i]);
                namelist[i] = NULL;
            }
            free(namelist);
            namelist = NULL; 
            return &files;      
        }        
    }
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

int create_folder(char *path, int permission)
{
    struct stat st = {0};
    if(path_ok)
    {
        if (stat(path, &st) == -1)
            if(permission != 0)
            {
                mkdir(path, permission);
                return FILE_OK;
            }
             else
            {
                mkdir(path,IMPLICIT_FOLDER_PERMISSION);
                return FILE_OK;
            }
        return FILE_E_CREATE;
    }
    else
        return FILE_E_PATH;
}

int remove_file(char *path)
{
    if(path_ok)
    {
        remove(path);
        return FILE_OK;
    }
     return FILE_E_PATH;
}

int create_file(char *path)
{
    if(path_ok)
    {
        FILE *f = fopen(path, "w");
        fclose(f);  
        return FILE_OK;  
    } 
    return FILE_E_PATH;   
}

int write_to_file(char *path, char *buf, int mode)
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
        else
            return FILE_E_WRITE;
        fclose(f); 
        return FILE_OK;
    }
    return FILE_E_PATH;
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