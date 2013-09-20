#include "make_options.h"

#define TIMEOUT 	5
#define MAX_READ	2048
#define MAKE_TAG 	"make"

int build_function(char *path, char **type) //type-error type
{											//function returns error code
	int pfd_output[2];
	int pfd_error[2];
	int rc = pipe(pfd_output);
	rc = pipe(pfd_error);
	chdir(path);
	int pid = fork();
	switch(pid)
	{
		case -1:
		{
			*type = "system";
			return MAKE_E_FORK;
		}
		case 0:
		{
			int rc = close(0);
			if(rc<0)						//FAC ASA SAU IGNOR????   SE MOSTENESTE TYPE???
				exit(rc);
			close(pfd_output[0]);
			close(pfd_error[0]);
			dup2(pfd_output[1],1);
			dup2(pfd_error[1],2);
			execl("/usr/bin/make", "make", "build");
			exit(-1);
		}
		default:
		{
			close(pfd_output[1]);
			close(pfd_error[1]);

			struct epoll_event ev;
			int epfd = epoll_create(2);

			ev.data.fd = pfd_output[0];
			ev.events = EPOLLIN;
			epoll_ctl(epfd, EPOLL_CTL_ADD, pfd_output[0], &ev);

			memeset(&ev, 0, sizeof(struct epoll_event));
			ev.data.fd = pfd_error[0];
			ev.events = EPOLLIN;
			epoll_ctl(epfd, EPOLL_CTL_ADD, pfd_error[0], &ev);

			struct epoll_event *ret_ev;
			memset(ret_ev,0,sizeof(struct epoll_event));

			char process_end=0;
			double timeout=0;
			time_t start_time=time(0);

			int return_code;
			
			while(timeout<TIMEOUT && process_end==0)
			{
				int nr = epoll_wait(efd, ret_ev, MAX_SHELLS, 0);
				if(nr > 0)
				{
					for (int i=0; i<nr; i++)
					{
						if(ret_ev[i].data.fd == pfd_output[0])
						{
							char *data = malloc(sizeof(char));
							int rc = read(pfd_output[0],data,MAX_READ);
							if (rc>0)
								send_build_tag(id,"stdout",data); //SE FACE FUNCTIA CARE TRIMITE SI FACE TAG-UL
							free(data);
						}
						else
						{
							char *data = malloc(sizeof(char));
							int rc = read(pfd_error[0],data,MAX_READ);
							if (rc>0)
								send_build_tag(id,"stderr",data);
							free(data);
						}
					}
					start_time=time(0);				
				}
				else
				{
					time_t end_time=time(0);
					timeout=difftime(end_time,start_time);
					start_time = time(0);
				}
				pid_t wait_pid = waitpid(pid,&return_code,WNOHANG);
				if(wait_pid == pid)
					process_end = 1;
			}
			xmpp_stanza_t *make = xmpp_stanza_new (wxmpp_get_context ());
            xmpp_stanza_set_name(make, MAKE_TAG);
            xmpp_stanza_set_ns (files, "wyliodrin");
            xmpp_stanza_set_attribute(files, "id", xmpp_stanza_get_attribute(stanza,"id"));//SE FACE ALA FINAL

		}
	}
}

void make_tag(const char *from, const char *to, int error, xmpp_stanza_t *stanza)
{
	char er[3]; //error code
	char *type; //error type(build return error/system error)
	if(error == 1)
        return;
	char *action = xmpp_stanza_get_attribute(stanza,"action");
	if(strncasecmp(action,"build",5)==0)
	{
		char *path = xmpp_stanza_get_attribute(stanza, "path");
		if(path == NULL)
		{
			sprintf(er,"%d", MAKE_E_PATH);
			type="system";
		}
		else
		{
			build_function(path, &type);
		}
	}
}