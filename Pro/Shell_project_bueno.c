/**
UNIX Shell Project

Sistemas Operativos
Grados I. Informatica, Computadores & Software
Dept. Arquitectura de Computadores - UMA

Some code adapted from "Fundamentos de Sistemas Operativos", Silberschatz et al.

To compile and run the program:
   $ gcc Shell_project.c job_control.c -o Shell
   $ ./Shell          
	(then type ^D to exit program)

**/
#include <stdio.h>
#include <string.h>
#include "job_control.h"
#define MAX_LINE 256 

job* lista;
typedef struct H_{
	char * command;
	struct H_* next;
}H;
H* listaH;
char* comando;

void jobs(){
	if(empty_list(lista)!=1){
		print_job_list(lista);
	}else{
		printf("La lista esta vacia \n");
	}
}


void cd(char * args[]){
	if(args[1]==NULL){
		printf("No se ha introducido el directorio.\n");
	}else if(args[2] != NULL){
		printf("Error en el formato de nombre del directorio.\n");	
	}else {
		if(chdir(args[1])==0){
			if(strcmp(args[1],"..")==0){
				printf("Volvemos al directorio anterior.\n");			
			}else{
				printf("Se ha accedido al directorio %s.\n", args[1]);				
			}				
		}else{
 			printf("El directorio %s no existe.\n", args[1]);				
		}
	}
}


void fg(char * args[]){
	int cont = 1, pid_wait, status;
	job * aux;
	if(lista->next == NULL){
		printf("No hay procesos suspendidos o en segundo plano.\n");
	}else if(args[1] != NULL && (list_size(lista)<atoi(args[1]) || 0>atoi(args[1]))){
			printf("La lista tiene menos procesos de los exigidos.\n");
	}else{
		if(args[1]!=NULL){
			cont=atoi(args[1]);
		}
		aux = get_item_bypos(lista,cont);
		set_terminal(aux->pgid);
		aux->state=FOREGROUND;
		killpg(aux->pgid,SIGCONT);
		pid_wait=waitpid(aux->pgid,&status,WUNTRACED); 
		if(pid_wait==aux->pgid){
			printf("El proceso %d ha terminado.\n",aux->pgid);
			cont=delete_job(lista,aux);
		}
		set_terminal(getpid());
	}
}


void bg(char * args[]){
	int cont = 1;
	job * aux;
	if(lista->next == NULL){
		printf("No hay procesos suspendidos o en segundo plano.\n");
	}else if(args[1] != NULL && (list_size(lista)<atoi(args[1]) || 0>atoi(args[1]))){
		printf("La lista tiene menos procesos de los exigidos.\n");
	}else{
		if(args[1]!=NULL){
			cont=atoi(args[1]);
		}
		aux = get_item_bypos(lista,cont);
		aux->state=BACKGROUND;
		killpg(aux->pgid,SIGCONT);
	}
}


void manejador (int signal){
	enum status status_res;
	int stat,info,pid_aux,auxiliar;
	job *aux1,*aux2;
	if((empty_list(lista))!=1){
		aux2=lista;
		aux1=aux2->next;
		while(aux1!=NULL){
			pid_aux=waitpid(aux1->pgid,&stat,WNOHANG|WUNTRACED);
			if(pid_aux==aux1->pgid){
				status_res=analyze_status(stat,&info);
				if(status_res!=SUSPENDED){
					printf("El proceso %d ha terminado correctamente.\n",aux1->pgid);
					auxiliar=delete_job(lista,aux1);
				}else{
					printf("El proceso %d esta suspendido.\n",aux1->pgid);	
					aux1->state=STOPPED;
					aux1=aux1->next;				
				}
			}else{
				aux1=aux1->next;
			}
		}
	}
}


H * nueva_lista(char * c){
	H* aux = (H *) malloc(sizeof(H));
	aux->command = c;
	return aux;
}


int tam (H * l){
	H* aux=l;
	int cont=0;
	while(aux!=NULL){
		cont++;
		aux=aux->next;
	}
	return cont;
}


H * new_command(const char * c){
	H * aux;
	aux = (H *) malloc(sizeof(H));
	aux->command = strdup(c);
	aux->next = NULL;
	return aux;
}


void almacenar(H * list, H * item){
	H * aux = list->next;
	list->next = item;
	item->next = aux;
}


H * get_command_bypos(H * l, int pos){
	H * aux = l;
	if(pos<1||pos>tam(l)) return NULL;
	pos++;
	while(aux->next != NULL && pos){
		aux= aux->next;
		pos--;
	}
	return aux;
}


void historial(char * args[]){
	int cont=1;
	H * aux;
	if(args[1] != NULL){
		if(tam(listaH)<atoi(args[1]) || 0>atoi(args[1])){
			printf("No hay suficientes comandos en el historial.\n");
		}else{
			cont = atoi(args[1]);
			aux = get_command_bypos(listaH,cont);
			//printf(aux->command);
			comando=aux->command;
		}
	}else{
		aux=listaH;
		printf("%s\n",aux->command);
		while(aux->next!=NULL){
			aux=aux->next;
			printf("%d	",cont);
			printf(aux->command);
			printf("\n");
			cont++;
		}
	}
}


int main(void)
{
	char inputBuffer[MAX_LINE];
	int background;
	char *args[MAX_LINE/2];
	int pid_fork, pid_wait;
	int status;
	enum status status_res;
	int info;
	ignore_terminal_signals();
	lista = new_list("procesos");
	listaH = nueva_lista("history");
	
	while (1)
	{   		
		signal(SIGCHLD,manejador);
		printf("\nCOMANDO-> ");
		fflush(stdout);
		get_command(inputBuffer, MAX_LINE, args, &background);
	
		if(args[0]==NULL) continue;
		
		almacenar(listaH,new_command(args[0]));

		if(strcmp(args[0],"historial")==0){
			historial(args);
			if(args[1]!=NULL){
				if((args[1]!=NULL)&&(strcmp(comando,"historial")==0)){
					printf(" -> ya está ejecutandose(justo arriba).\n");
					continue;
				}else{
					args[0]=comando;
					args[1]=NULL;
				}
			}
		}
		if(strcmp(args[0],"historial")==0){		
			continue;
		}
		if(strcmp(args[0],"cd")==0){
				cd(args);		
		}else if(strcmp(args[0],"jobs")==0){
				jobs();		
		}else if(strcmp(args[0],"bg")==0){
				bg(args);		
		}else if(strcmp(args[0],"fg")==0){
				fg(args);	
		}else{
			block_SIGCHLD();
			pid_fork=fork();
			if(pid_fork==0){
				new_process_group(getpid());
				if(background!=1){
					set_terminal(getpid());
				}
				restore_terminal_signals();
				execvp(args[0],args);
				printf("Error, Command not found: %s\n",args[0]);
				exit(1);
			 }else{			
				if(background==1){
					add_job(lista,new_job(pid_fork,args[0],background));
					printf("Background job running... pid: %d, command: %s\n",pid_fork,args[0]);
					unblock_SIGCHLD();
				}else{
					set_terminal(pid_fork);
					pid_wait=waitpid(pid_fork,&status,WUNTRACED);
					status_res=analyze_status(status,&info);
					set_terminal(getpid());
					if(info != 1){
						printf("Foreground pid: %d, command: %s %s info: %d\n"
                        ,pid_fork,args[0],status_strings[status_res],info);
					}
					if(status_strings[status_res] == "Suspended"){
						add_job(lista,new_job(pid_fork,args[0],STOPPED));
						printf("Proceso %d suspendido.\n",getpid());
						unblock_SIGCHLD();
					}
				}
			
			}
		}
		
	}
}
