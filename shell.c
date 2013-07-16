#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>

#include "shell.h"
 

static shell* shell_list[MAX_SHELLS];	
static int efd;
static char *received_keys;
static int epoll_events = MAX_SHELLS;
pthread_mutex_t mutex_id, mutex_write;

static keys_pressed function_keys_pressed = NULL;

static pthread_t shell_thread;

static int shell_exit = 0;

shell* find_shell_fd(int fd, int *write)
{
	int found = 0;
	int i = 0;
	*write = 0;
	while((i<MAX_SHELLS) && (found == 0))
	{
		//printf("fdm = %d\n", fdm);
		//printf("sh->fdm = %d\n",shell_list[0]->fdm);
		//printf("shell[0]
		//printf("i=%d\n",i);
			// printf("shell fd = %d\n",shell_list[i]->fdm);
		if (shell_list[i] != NULL)
		{
			if(shell_list[i]->fdm == fd)
				found = 1;
			else if(shell_list[i]->fdm_write == fd)
			{
				found = 1;
				*write = 1;
			}
			else
				i++;
		}
		else
			i++;
	}
		if (found == 0)
			//return SHELL_E_ID_NOT_FOUND;
			return NULL;
		else
			return shell_list[i];
}

shell* find_shell_id(int id)
{
	int found = 0;
	int i = 0;
	while(i<MAX_SHELLS && found == 0)
	{
		//printf("fdm = %d\n", fdm);
		//printf("sh->fdm = %d\n",shell_list[0]->fdm);
		//printf("shell[0]
		if ((shell_list[i] != NULL) && (shell_list[i]->id == id))
			found = 1;
		else
			i++;
	}
		if (found == 0)
			//return SHELL_E_ID_NOT_FOUND;
			return NULL;
		else
			return shell_list[i];
}



int init_shells ()
{
	int i;

	efd = epoll_create(MAX_SHELLS * 2);
	if(efd < 0)
		return SHELL_EPOLL_CREATE;
	for(i=0; i<MAX_SHELLS; i++)
	{ 
		shell_list[i] = NULL;
	}
	pthread_mutex_init(&mutex_id, NULL);
	pthread_mutex_init(&mutex_write, NULL);
	received_keys = malloc(MAX_KEYS * sizeof(char));
	if (received_keys == NULL)
		return SHELL_MALLOC;
	return SHELL_OK; 
}

shell * create_shell_id()
{
	pthread_mutex_lock(&mutex_id);
	int i = 0;
	int found = 0;
	while(i<MAX_SHELLS && found == 0)
	{
		if(shell_list[i] == NULL)
		{
			//printf("malloc\n");
			found = 1;
			shell_list[i] = malloc(sizeof(struct s_shell));
			if(shell_list[i] == NULL)
			{
				//printf("return null\n");
				return NULL;//SHELL_MALLOC;
			}
			else
			{
				shell_list[i]->id = i;
			}
		}
		else
		{
			i++;
		}
	}
	pthread_mutex_unlock(&mutex_id);
	if(found == 0)
		return NULL;//SHELL_LIST_FULL;
	//printf("sh[0] = %p\n",shell_list[0]);
	return shell_list[i];
}

int create_shell()
{
	//int id;
	pid_t pid;
	struct termios term, term_orig;
	int fdm,fds;
	int rc;
	shell *current_shell;
	
	if((current_shell = create_shell_id()) != NULL)
	{
		/*current_shell->keys = malloc(MAX_KEYS * sizeof(char));
		if(current_shell->keys == NULL)
			return SHELL_MALLOC;

		current_shell->received_message = malloc(MAX_KEYS * sizeof(char));
		if(current_shell->received_message == NULL)
			return SHELL_MALLOC;*/

		current_shell->sent_keys = malloc (MAX_KEYS * sizeof(char));
		if (current_shell->sent_keys == NULL)
			return SHELL_MALLOC;

		current_shell->no_mem_keys = 0;
		current_shell->in_epoll = 0;

		int m, s;
		struct winsize w = {24, 80, 0, 0};

		/* seems to work fine on linux, openbsd and freebsd */
		if(openpty(&fdm, &fds, NULL, NULL, &w) < 0)
		{
			return SHELL_OPENPT;
		}

		// fdm = posix_openpt(O_RDWR);
		// if (fdm < 0)
		// {
		// 	return SHELL_OPENPT;
		// }
		
		// rc = grantpt(fdm);
		// if (rc != 0)
		// {
		// 	return SHELL_GRANDTPT;
		// }
		
		// rc = unlockpt(fdm);
		// if (rc != 0)
		// {
		// 	return SHELL_UNLOCK;
		// }
		// /* Open the slave side ot the PTY*/
		// fds = open(ptsname(fdm), O_RDWR);
		// if(fds < 0)
		// {
		// 	return SHELL_OPEN;
		// }
		/* Create the child process */
		pid = fork();
		switch (pid) 
		{
			case -1:
				/* error forking */
				return SHELL_FORK;

			case 0:		
				/* child process */
		
				/* Close the master side of the PTY*/
				close(fdm);

				/* The slave side of the PTY becomes the standard input and outputs of the child process */
				close(0); // Close standard input (current terminal)
				close(1); // Close standard output (current terminal)
				close(2); // Close standard error (current terminal)

				dup2(fds,0); // PTY becomes standard input (0)
				dup2(fds,1); // PTY becomes standard output (1)
				dup2(fds,2); // PTY becomes standard error (2)
		
				/* Now the original file descriptor is useless*/
				close(fds);
		
				/* Make the current process a new session leader*/
				setsid();

				/* As the child is a session leader, set the controlling terminal to be the slave side of the PTY
				(Mandatory for programs like the shell to make them manage correctly their outputs)*/
				ioctl(0, TIOCSCTTY, 1);
		
				/*Start cmd.*/
				execl(COMMAND, COMMAND, NULL);
				/* only if exec failed */
				return SHELL_EXECV;
				
			default:
					/* parent process */
			/* Close the slave side of the PTY*/
			close(fds);
			int flags = fcntl(fdm, F_GETFL, 0);
			fcntl(fdm, F_SETFL, flags | O_NONBLOCK);

			int fd;
			if(fd = dup(fdm) < 0)
				return SHELL_DUP;

			current_shell->fdm = fdm;
			current_shell->fdm_write = fd;
			current_shell->pid = pid;

			struct epoll_event ev;

			ev.data.fd = current_shell->fdm;       
			ev.events = EPOLLIN;
			if(epoll_ctl(efd, EPOLL_CTL_ADD, current_shell->fdm, &ev) != 0)
				return SHELL_EPOLL_CTL;		
		}
		return current_shell->id;
	}
	else
		return SHELL_ECREATE;
}

void * shell_start(void * data)
{
	int rc;
	int write;

	while(!shell_exit)
	{
		// printf("while\n");
		struct epoll_event ret_ev;
		memset (&ret_ev, 0, sizeof (struct epoll_event));
		int r = epoll_wait(efd, &ret_ev, MAX_SHELLS, -1);
		if (r > 0)
		{
			// printf("fd=%d\n",ret_ev.data.fd);
			shell * current_shell = find_shell_fd(ret_ev.data.fd, &write);
			if (current_shell != NULL)
			{
				if (write == 0)
				{
					rc = read(current_shell->fdm, received_keys, MAX_KEYS-1);
					//received_keys[MAX_KEYS-1] = '\0';
					if (rc > 0)
		          	{
		            // Send data on standard output

		          		// int i;
		          		// for (i=0; i<rc; i++)
		          		// {
		            // 		putc(received_keys[i], stdout);
		            // 	}
		            // 	fflush (stdout);
		            	if (function_keys_pressed != NULL)
	            		{
	            			function_keys_pressed (current_shell->id, received_keys, rc);
	            		}
		          	}
		          	else
		          	{
		            	if (rc < 0)
		            	{
		              		// TODO log this
		            	}
		          	}
				}
				else
				{
					write_keys(current_shell,"",0);
				}
				
			}
		}
        else perror ("epoll_wait");
	}
	return NULL;
}

int run_shell ()
{
	if (pthread_create (&shell_thread, NULL, shell_start, NULL)<0)
	{
		return SHELL_THREAD;
	}
	return SHELL_OK;
}

int write_keys(shell * sh, char * buf, int n)
{
	pthread_mutex_lock(&mutex_write);
	int no_written;
	int full = 0;
	if((n + sh->no_mem_keys) > MAX_KEYS)
	{
		sh->sent_keys = memcpy(sh->sent_keys + sh->no_mem_keys, buf, MAX_KEYS - sh->no_mem_keys);
		sh->no_mem_keys = MAX_KEYS;
		full = 1;
	}
	else
	{
		sh->sent_keys = memcpy(sh->sent_keys + sh->no_mem_keys, buf, n);
		sh->no_mem_keys = sh->no_mem_keys + n;
	}
	no_written = write(sh->fdm, sh->sent_keys, sh->no_mem_keys);
	if(no_written < 0)
	{
		//TODO write error
	}
	else
	{
		if(no_written == sh->no_mem_keys)
		{
			memset(sh->sent_keys, 0, sh->no_mem_keys);
			sh->no_mem_keys = 0;			
			if(sh->in_epoll == 1)
			{
				epoll_ctl(efd, EPOLL_CTL_DEL, sh->fdm_write, NULL);
				sh->in_epoll = 0;
				epoll_events--;	
			}			
		}
		else
		{
			sh->no_mem_keys = sh->no_mem_keys - no_written;
			sh->sent_keys = memmove(sh->sent_keys,sh->sent_keys+no_written, sh->no_mem_keys);
			if(sh->in_epoll == 0)
			{
				struct epoll_event ev;
				ev.data.fd = sh->fdm_write;
				ev.events = EPOLLOUT;
				if(epoll_ctl(efd, EPOLL_CTL_ADD, sh->fdm_write, &ev) != 0)
					//TODO epoll add	
				epoll_events++;
				sh->in_epoll = 1;
			}
		}
	}
	pthread_mutex_unlock(&mutex_write);
	if (full == 1)
		return SHELL_BUFFER_FULL;
	else
		return SHELL_OK;
}

int send_keys_to_shell(int id, char * buf, int n)
{
	int rc;
	shell * current_shell = find_shell_id(id);

	if(current_shell != NULL)
	{
		 return write_keys(current_shell, buf, n);
	}
	else
		return SHELL_NULL;
}
			/*
				if (ret_ev.data.fd == STDIN_FILENO)
				 {
					// printf("typed\n");
					rc = read(0, character,1);
					if(rc > 0)
					{
						// Send data on the master side of PTY
						write(fdm, character, rc);
					}
					else
					{
						if(rc < 0)
						{
							return SHELL_WRITE;
						}
					}
				 }
			}
			break;*/
void * start_th()
{
	send_keys_to_shell(0, "ls\n",4);
}

void set_keys_pressed (keys_pressed keys)
{
	function_keys_pressed = keys;
}

int send_signal_terminal(int pid, int signal, int times)
{
	int i;
	int status;
	for(i=0; i<times; i++)
	{
		kill(pid,signal);
		if(waitpid(pid,&status,WNOHANG) == pid)
			return SHELL_OK;
		usleep(100);
	}
	return SHELL_TERM;
}
void close_shell(int id)
{
	shell * current_shell = find_shell_id(id);
	if (current_shell == NULL) return;
	free(current_shell->sent_keys);
	if(current_shell->in_epoll == 1)
	{
		epoll_ctl(efd, EPOLL_CTL_DEL, current_shell->fdm_write, NULL);
		epoll_events--;
	}
	epoll_ctl(efd, EPOLL_CTL_DEL, current_shell->fdm, NULL);
	if(close(current_shell->fdm_write)<0)
		//TODO log this
	if(close(current_shell->fdm)<0)
		//TODO log this
	if(send_signal_terminal(current_shell->pid, SIGTERM, 5) != SHELL_OK)
	{
		send_signal_terminal(current_shell->pid, SIGKILL, 2);
	}
	free(current_shell);
	shell_list[id] = NULL;
}

void close_session()
{
	free(received_keys);
	pthread_mutex_destroy(&mutex_id);
	pthread_mutex_destroy(&mutex_write);
}

/*int main()
{
	printf("main\n");
	init_shells();
	int id = create_shell();
	pthread_t fir1;
	if (pthread_create(&fir1, NULL, &start_th, NULL)) {
		perror("pthread_create");
		exit(1);
	}
	return run_shell();
	//return 0;
}*/