#include "xmpp_file.h"

#define FILES_TAG	files

xmpp_stanza_t *list(xmpp_stanza_t *stanza)
{
	int n;
	char *path = xmpp_stanza_get_attribute(stanza,"path");
	if(path == NULL)
	{
		path = malloc(sizeof(char));
		path = "/";
	}
	files_list **f_list = list_files(path, &n);
	if(f_list == NULL)
	{
		xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
		xmpp_stanza_set_name(files, FILES_TAG);
		xmpp_stanza_set_ns (shells, "wyliodrin");
		xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));
		xmpp_stanza_set_attribute(files, "error", FILE_E_UNKNOWN);
		return files;
	}
	else
	{
		int i;
		xmpp_stanza_t *files = xmpp_stanza_new (wxmpp_get_context ());
		xmpp_stanza_set_name(files, FILES_TAG);
		xmpp_stanza_set_ns (shells, "wyliodrin");
		xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));
		xmpp_stanza_set_attribute(files, "error", FILE_E_OK);
		for(i=0; i<n; i++)
		{	
			xmpp_stanza_t *child = xmpp_stanza_new (wxmpp_get_context());
			switch (f_list[i]->type)
	        {
	            case FIFO:				xmpp_stanza_set_name(child, "pipe");                  break;
	            case CHARACTER_DEVICE:  xmpp_stanza_set_name(child, "character_device");      break;
	            case DIRECTORY:   		xmpp_stanza_set_name(child, "directory");             break;
	            case BLOCK_DEVICE:   	xmpp_stanza_set_name(child, "block_device");          break;
	            case REGULAR_FILE:   	xmpp_stanza_set_name(child, "file");         		  break;
	            case SYMLINK:   		xmpp_stanza_set_name(child, "link");             	  break;
	            case SOCKET:   			xmpp_stanza_set_name(child, "socket");                break;
	            default:   				xmpp_stanza_set_name(child, "unknown");               break;
	        }
	        xmpp_stanza_set_attribute(childe,"name",f_list[i]->name);
	        xmpp_stanza_add_child(files,child);
	        xmpp_stanza_release(child);
		}
		return files;
	}
}

void file_tag(const char *from, const char *to, int error, xmpp_stanza_t *stanza)
{
	if(error == 1)
		return;
	char *action = xmpp_stanza_get_attribute (stanza, "action");
	if (action != NULL)
	{
		if (strncasecmp (action, "list", 4)==0)
		{
			xmpp_stanza_t *f = list(stanza);
			wxmpp_send("alex@wyliodrin.com", f);
			xmpp_stanza_release(f);
			f = NULL;
		}
	}
}