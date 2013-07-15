#ifndef SHELL_H_
#define SHELL_H_

#define EPOLL_FD		2
#define MAX_SHELLS		100
#define MAX_KEYS		101
#define COMMAND			"/bin/bash"
/*Defining errors*/
#define SHELL_OK		0
#define SHELL_FORK		-1
#define	SHELL_EXECV		-2
#define SHELL_OPENPT	-3
#define SHELL_GRANDTPT	-4
#define SHELL_UNLOCK	-5
#define SHELL_OPEN		-6
#define SHELL_WRITE		-7
#define SHELL_MALLOC	-8
#define SHELL_LIST_FULL	-9
#define SHELL_RCGETATTR	-10
#define SHELL_E_ID_NOT_FOUND	-11
#define SHELL_ECREATE	-12
#define SHELL_EPOLL_CREATE	-13
#define SHELL_NULL	-14
#define SHELL_EPOLL_CTL	-15
#define SHELL_IO_SUBMIT	-16
#define SHELL_READ	-17
#define SHELL_BUFFER_FULL	-18
#define SHELL_DUP	-19
	


typedef struct s_shell{
	int id;
	int fdm;
	int fdm_write;
	char *sent_keys;
	int no_mem_keys;
	int in_epoll;
	} shell;
	
typedef void (*keys_pressed)(int id, char *keys, int n);

int init_shells();
int create_shell();
int run_shell();
int send_keys_to_shell(int id, char * buf, int n);
void set_keys_pressed (keys_pressed keys);
	
#endif