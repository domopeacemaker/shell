/*--------------------------------------------------------
UNIX Shell Project
job_control module
Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA
Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.
--------------------------------------------------------*/

#include "job_control.h"
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <string.h>


void get_command(char inputBuffer[], int size, char *args[],int *background)
{
	int length, 
		i,      
		start, 
		ct;  

	ct = 0;
	*background=0;

	
	length = read(STDIN_FILENO, inputBuffer, size);  

	start = -1;
	if (length == 0)
	{
		printf("\nGracias pixa por usar la Shell AMCS(R) v1.4\n\n");
		exit(0);           
	} 
	if (length < 0){
		perror("error reading the command");
		exit(-1);           
	}

	for (i=0;i<length;i++) 
	{ 
		switch (inputBuffer[i])
		{
		case ' ':
		case '\t' :           
			if(start != -1)
			{
				args[ct] = &inputBuffer[start];   
				ct++;
			}
			inputBuffer[i] = '\0';
			start = -1;
			break;

		case '\n':                
			if (start != -1)
			{
				args[ct] = &inputBuffer[start];     
				ct++;
			}
			inputBuffer[i] = '\0';
			args[ct] = NULL; 
			break;

		default :        

			if (inputBuffer[i] == '&')
			{
				*background  = 1;
				if (start != -1)
				{
					args[ct] = &inputBuffer[start];     
					ct++;
				}
				inputBuffer[i] = '\0';
				args[ct] = NULL; 
				i=length;

			}
			else if (start == -1) start = i;  
		}  
	}  
	args[ct] = NULL; 
} 


job * new_job(pid_t pid, const char * command, enum job_state state)
{
	job * aux;
	aux=(job *) malloc(sizeof(job));
	aux->pgid=pid;
	aux->state=state;
	aux->command=strdup(command);
	aux->next=NULL;
	return aux;
}

void add_job (job * list, job * item)
{
	job * aux=list->next;
	list->next=item;
	item->next=aux;
	list->pgid++;

}

int delete_job(job * list, job * item)
{
	job * aux=list;
	while(aux->next!= NULL && aux->next!= item) aux=aux->next;
	if(aux->next)
	{
		aux->next=item->next;
		free(item->command);
		free(item);
		list->pgid--;
		return 1;
	}
	else
		return 0;

}

job * get_item_bypid  (job * list, pid_t pid)
{
	job * aux=list;
	while(aux->next!= NULL && aux->next->pgid != pid) aux=aux->next;
	return aux->next;
}
// -----------------------------------------------------------------------
job * get_item_bypos( job * list, int n)
{
	job * aux=list;
	if(n<1 || n>list->pgid) return NULL;
	n--;
	while(aux->next!= NULL && n) { aux=aux->next; n--;}
	return aux->next;
}

void print_item(job * item)
{

	printf("pid: %d, command: %s, state: %s\n", item->pgid, item->command, state_strings[item->state]);
}

void print_list(job * list, void (*print)(job *))
{
	int n=1;
	job * aux=list;
	printf("Contents of %s:\n",list->command);
	while(aux->next!= NULL) 
	{
		printf(" [%d] ",n);
		print(aux->next);
		n++;
		aux=aux->next;
	}
}

enum status analyze_status(int status, int *info)
{
	if (WIFSTOPPED (status))
	{
		*info=WSTOPSIG(status);
		return(SUSPENDED);
	}
	else
	{
		// el proceso termio
		if (WIFSIGNALED (status))
		{ *info=WTERMSIG (status); return(SIGNALED);}
		else
		{ *info=WEXITSTATUS(status); return(EXITED);}
	}
	return;
}

void terminal_signals(void (*func) (int))
{
	signal (SIGINT,  func);
	signal (SIGQUIT, func);
	signal (SIGTSTP, func);
	signal (SIGTTIN, func);
	signal (SIGTTOU, func);
}		

// -----------------------------------------------------------------------
void block_signal(int signal, int block)
{
	sigset_t block_sigchld;
	sigemptyset(&block_sigchld );
	sigaddset(&block_sigchld,signal);
	if(block)
	{
		sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
	}
	else
	{
		sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
	}
}

