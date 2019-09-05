/**
UNIX Shell Project
Autor: Juan Alvaro Caravaca Seliva, Computadores A

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/

#include "job_control.h"   // remember to compile with module job_control.c 
#include <string.h>

#define MAX_LINE 256 /* 256 chars per line, per command, should be enough. */

job* job_list;

void manejador(int senal){
	int i, status, pid_wait, info;
	enum status status_res;
	for(i=list_size(job_list); i>0; i--){	///block_SIGCHLD???		for(i=1; i<=list_size(job_list); i++)
		job* elem = get_item_bypos(job_list, i);
		pid_wait = waitpid(elem->pgid, &status, WUNTRACED|WNOHANG);
		if(pid_wait == elem->pgid){
			status_res = analyze_status(status, &info);
			printf("\n  Background pid: %d, command: %s, status: %s, info: %d\n\n", pid_wait, elem->command, status_strings[status_res], info);
			fflush(stdout);
			if(status_res == SUSPENDED)
				elem->state = STOPPED;
			else {
				printf("\n  Eliminando tarea %s con pid: %i\n\n", elem->command, elem->pgid);
				delete_job(job_list, elem);
			}
			printf("COMMAND-> ");
			fflush(stdout);
		}
	}
}



// -----------------------------------------------------------------------
//                            MAIN          
// -----------------------------------------------------------------------

int main(void)
{
	char inputBuffer[MAX_LINE]; /* buffer to hold the command entered */
	int background;             /* equals 1 if a command is followed by '&' */
	char *args[MAX_LINE/2];     /* command line (of 256) has max of 128 arguments */
	// probably useful variables:
	int pid_fork, pid_wait; /* pid for created and waited process */
	int status;             /* status returned by wait */
	enum status status_res; /* status processed by analyze_status() */
	int info;				/* info processed by analyze_status() */
	
	job_list = new_list("Tareas Shell");
	

	ignore_terminal_signals();
	signal(SIGCHLD, manejador);
	printf("\n");
	while (1)   /* Program terminates normally inside get_command() after ^D is typed*/
	{   		
		printf("COMMAND-> ");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);  /* get next command */
		
		if(args[0]==NULL) continue;   // if empty command
		
		if(strcmp(args[0], "cd")==0){
			int infoCd = chdir(args[1]);
			if (infoCd==0) {
				getcwd(args[1], MAX_LINE);
				printf("%s\n\n", args[1]);
			} else printf("  Error ejecutando cd al path: %s\n\n", args[1]);
			continue;
		}
		
		if(strcmp(args[0], "jobs")==0){
			if(empty_list(job_list)) printf("La lista esta vacia\n\n");
			else {
				print_job_list(job_list);
				printf("\n");
			}
			continue;
		}
		
		if(strcmp(args[0], "fg")==0){
			int i;
			if(args[1]==NULL) i=1;
			else {
				if(atoi(args[1])<=0) {
					printf("Argumento %s no valido\n\n", args[1]);
					continue;
				}
				else i = atoi(args[1]);
			}
			block_SIGCHLD();
			job* elem = get_item_bypos(job_list, i);
			elem->state = FOREGROUND;
			/**printf("justo despues elem\n");
			fflush(stdout);
			int j_pgid = elem->pgid;
			printf("justo despues pgid\n");
			fflush(stdout);
			char* j_command;
			memset(j_command, '\0', sizeof(elem->command));
			strcpy(j_command, elem->command);
			printf("justo despues command\n");
			fflush(stdout);
			enum job_state j_status = elem->state;
			printf("justo antes\n");
			fflush(stdout);
				delete_job(job_list, elem);
			printf("justo despues\n");
			fflush(stdout);*/
			unblock_SIGCHLD();
			
			set_terminal(elem->pgid);
			killpg(elem->pgid, SIGCONT);
			pid_wait = waitpid(elem->pgid, &status, WUNTRACED);	///Se podria borrar del codigo pid_wait?
			set_terminal(getpid());
			status_res = analyze_status(status, &info);
			printf("\n  Foreground pid: %d, command: %s, status: %s, info: %d\n\n", elem->pgid, elem->command, status_strings[status_res], info);
			if(status_res == SUSPENDED) {
				///printf("El proceso %i se ha suspendido\n\n", elem->pgid);
				block_SIGCHLD();
				elem->state = STOPPED;
				unblock_SIGCHLD();
			} 
			if(status_res == EXITED) {
				block_SIGCHLD();
				delete_job(job_list, elem);
				unblock_SIGCHLD();
				
			}
			
			continue;
		}
		
		if(strcmp(args[0], "bg")==0){
			if(atoi(args[1])<=0) {
				printf("Argumento %s no valido\n\n", args[1]);
				continue;
			}
			block_SIGCHLD();
			job* elem = get_item_bypos(job_list, atoi(args[1]));
			elem->state = BACKGROUND;
			killpg(elem->pgid, SIGCONT);
			printf("Proceso %s con pid %i puesto en background\n\n", elem->command, elem->pgid);
			unblock_SIGCHLD();
			continue;
		}
		
		///streduct le pasas job->command stldup
		/* the steps are:
			 (1) fork a child process using fork()
			 (2) the child process will invoke execvp()
			 (3) if background == 0, the parent will wait, otherwise continue 
			 (4) Shell shows a status message for processed command 
			 (5) loop returns to get_commnad() function
		*/
		pid_fork = fork();
		if(pid_fork != 0){	//Padre
			new_process_group(pid_fork);
			if(background){
				printf("Padre: proceso hijo %i en background\n\n", pid_fork);
				block_SIGCHLD();
				add_job(job_list, new_job(pid_fork, args[0], background));
				unblock_SIGCHLD();
			} else {
				set_terminal(pid_fork);
				pid_wait = waitpid(pid_fork, &status, WUNTRACED);
				///AÑADIR A JOB_LIST PROCESOS HIJOS QUE SE SUSPENDAN. COMO PRUEBO LA SUSPENSION?
				set_terminal(getpid());
				status_res = analyze_status(status, &info);
				if(status_res == SUSPENDED) {
					printf("Padre: proceso hijo %i suspendido\n\n", pid_fork);
					block_SIGCHLD();
					add_job(job_list, new_job(pid_fork, args[0], STOPPED));
					unblock_SIGCHLD();
				}
				printf("\n  Foreground pid: %d, command: %s, status: %s, info: %d\n\n", pid_fork, args[0], status_strings[status_res], info);
			}
		} else {	//Hijo
			new_process_group(getpid());
			restore_terminal_signals();
			if(!background){
				set_terminal(getpid());
			}
			execvp(args[0], args);
			printf("\n  Error en hijo");
			exit(1);
		}

	} // end while
}
