#include "files.h"

#define FILES_TAG   "files"

files_list *list_files(char *path, int *n)
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
                files[i].name = namelist[i]->d_name;
                switch (namelist[i]->d_type)
                {
                    case DT_FIFO:   files[i].type = FIFO;                  break;
                    case DT_CHR :   files[i].type = CHARACTER_DEVICE;      break;
                    case DT_DIR :   files[i].type = DIRECTORY;             break;
                    case DT_BLK :   files[i].type = BLOCK_DEVICE;          break;
                    case DT_REG :   files[i].type = REGULAR_FILE;          break;
                    case DT_LNK :   files[i].type = SYMLINK;               break;
                    case DT_SOCK:   files[i].type = SOCKET;                break;
                    default     :   files[i].type = UNKNOWN;               break;
                }

                free(namelist[i]);
                namelist[i] = NULL;
            }
            free(namelist);
            namelist = NULL; 
            return files;      
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
                return FILE_E_OK;
            }
             else
            {
                mkdir(path,IMPLICIT_FOLDER_PERMISSION);
                return FILE_E_OK;
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
        return FILE_E_OK;
    }
     return FILE_E_PATH;
}

int create_file(char *path)
{
    if(path_ok)
    {
        FILE *f = fopen(path, "w");
        fclose(f);  
        return FILE_E_OK;  
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
        return FILE_E_OK;
    }
    return FILE_E_PATH;
}

xmpp_stanza_t *list(xmpp_stanza_t *stanza)
{
    int n;
    char er[3];
    char *path = xmpp_stanza_get_attribute(stanza,"path");
    if(path == NULL)
    {
        path = malloc(sizeof(char));
        path = "/";
    }
    files_list *f_list = list_files(path, &n);
    if(f_list == NULL)
    {
        xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
        xmpp_stanza_set_name(files, FILES_TAG);
        xmpp_stanza_set_ns (files, "wyliodrin");
        xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));
        sprintf(er,"%d",FILE_E_UNKNOWN);
        xmpp_stanza_set_attribute(files, "error", er);
        return files;
    }
    else
    {
        int i;
        xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
        xmpp_stanza_set_name(files, FILES_TAG);
        xmpp_stanza_set_ns (files, "wyliodrin");
        xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));
        sprintf(er,"%d",FILE_E_OK);
        xmpp_stanza_set_attribute(files, "error", er);
        for(i=0; i<n; i++)
        {   
            xmpp_stanza_t *child = xmpp_stanza_new (wxmpp_get_context());
            switch (f_list[i].type)
            {
                case FIFO:              xmpp_stanza_set_name(child, "pipe");                  break;
                case CHARACTER_DEVICE:  xmpp_stanza_set_name(child, "character_device");      break;
                case DIRECTORY:         xmpp_stanza_set_name(child, "directory");             break;
                case BLOCK_DEVICE:      xmpp_stanza_set_name(child, "block_device");          break;
                case REGULAR_FILE:      xmpp_stanza_set_name(child, "file");                  break;
                case SYMLINK:           xmpp_stanza_set_name(child, "link");                  break;
                case SOCKET:            xmpp_stanza_set_name(child, "socket");                break;
                default:                xmpp_stanza_set_name(child, "unknown");               break;
            }
            xmpp_stanza_set_attribute(child,"name",f_list[i].name);
            xmpp_stanza_add_child(files,child);
            xmpp_stanza_release(child);
            free(f_list[i].name);
        }
        free(f_list);
        return files;
    }
}

void file_tag(const char *from, const char *to, int error, xmpp_stanza_t *stanza)
{
    char er[3];
    if(error == 1)
        return;
    char *action = xmpp_stanza_get_attribute (stanza, "action");
    if (action != NULL)
    {
        if (strncasecmp (action, "list", 4)==0)
        {
            xmpp_stanza_t *files = list(stanza);
            wxmpp_send("alex@wyliodrin.com", files);
            xmpp_stanza_release(files);
        }
        else if(strncasecmp(action,"delete", 6) == 0)
        {    
            char *path = xmpp_stanza_get_attribute(stanza, "path");
 
            xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
            xmpp_stanza_set_name(files, FILES_TAG);
            xmpp_stanza_set_ns (files, "wyliodrin");
            xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));

            if(path == NULL)
                sprintf(er,"%d",FILE_E_PATH); 
            else
                sprintf(er,"%d",delete(path));

            xmpp_stanza_set_attribute(files, "error", er);
            wxmpp_send("alex@wyliodrin.com", files);
            xmpp_stanza_release(files);
        }
        else if (strncasecmp(action, "create", 6) == 0)
        {
            char *type = xmpp_stanza_get_attribute(stanza, "type");
            xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
            xmpp_stanza_set_name(files, FILES_TAG);
            xmpp_stanza_set_ns (files, "wyliodrin");
            xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));

            char *path = xmpp_stanza_get_attribute(stanza, "path");
            if(path == NULL)
                sprintf(er,"%d",FILE_E_PATH); 
            else
            {
                if(strncasecmp(type, "file", 4) == 0)
                    sprintf(er,"%d",create_file(path));
                else
                {
                    char *perm = xmpp_stanza_get_attribute(stanza,"permissions");
                    int p;
                    if(perm == NULL)
                        p=0;
                    else
                        p=atoi(perm);

                    sprintf(er,"%d",create_folder(path,p));
                }

            }
            xmpp_stanza_set_attribute(files,"error",er);
            wxmpp_send("alex@wyliodrin.com", files);
            xmpp_stanza_release(files);
        }
        else if (strncasecmp(action, "write", 5) == 0)
        {
            xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
            xmpp_stanza_set_name(files, FILES_TAG);
            xmpp_stanza_set_ns (files, "wyliodrin");
            xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));

            char *path = xmpp_stanza_get_attribute(stanza, "path");
            if(path == NULL)
                sprintf(er,"%d",FILE_E_PATH); 
            else
            {
                char *mode = xmpp_stanza_get_attribute(stanza, "mode");
                if(mode == NULL)
                    sprintf(er,"%d", FILE_E_MODE);
                else
                {
                    char *buf = xmpp_stanza_get_text (stanza);
                    char m;
                    m=(strncasecmp(mode,"a",1)==0)? APPEND : WRITE;
                    sprintf(er,"%d", write_to_file(path,buf,m));
                }
            }
            xmpp_stanza_set_attribute(files,"error",er);
            wxmpp_send("alex@wyliodrin.com", files);
            xmpp_stanza_release(files);
        }
        else if(strncasecmp(action, "details",7))
        {
            xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
            xmpp_stanza_set_name(files, FILES_TAG);
            xmpp_stanza_set_ns (files, "wyliodrin");
            xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));

            char *path = xmpp_stanza_get_attribute(stanza, "path");
            if(path == NULL)
                sprintf(er,"%d",FILE_E_PATH);
            else
            {
                file *f = get_file_info(path);
                switch (f->type)
                {
                    case FIFO:              xmpp_stanza_set_attribute(files, "type","pipe");                  break;
                    case CHARACTER_DEVICE:  xmpp_stanza_set_attribute(files, "type","character_device");      break;
                    case DIRECTORY:         xmpp_stanza_set_attribute(files, "type","directory");             break;
                    case BLOCK_DEVICE:      xmpp_stanza_set_attribute(files, "type","block_device");          break;
                    case REGULAR_FILE:      xmpp_stanza_set_attribute(files, "type","file");                  break;
                    case SYMLINK:           xmpp_stanza_set_attribute(files, "type","link");                  break;
                    case SOCKET:            xmpp_stanza_set_attribute(files, "type","socket");                break;
                    default:                xmpp_stanza_set_attribute(files, "type","unknown");               break;
                }
                xmpp_stanza_set_attribute(files,"atime", ctime(f->atime));
                xmpp_stanza_set_attribute(files,"stime", ctime(f->stime));
                xmpp_stanza_set_attribute(files,"mtime", ctime(f->mtime));
                char *size = malloc(sizeof(char));
                sprintf(size,"%l", f->size);
                xmpp_stanza_set_attribute(files,"size", size);
                free(size);
                char *permission = malloc(sizeof(char));
                sprintf(permission, "%i", f->permission);
                xmpp_stanza_set_attribute(files, "permissions", permission);
                free(permission);
                sprintf(er, "%d", FILE_E_OK);
            }
            xmpp_stanza_set_attribute(files, "error", er);
            wxmpp_send("alex@wyliodrin.com", files);
            xmpp_stanza_release(files);
        }
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