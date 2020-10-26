#include "Shell.h"
#include "Evaluation.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

/*
 * Will create a child processus to execute a command
 * with execvp with the command and args
 * passed in parameters
 * */
int execute_simple_command(char *command, char **args, bool background){
	int pid  = fork();
	if(!pid){
		execvp(command, args);
		exit(EXIT_FAILURE); // if execvp fails it return -1 we could also return the return of execvp but this line would execute only if execvp cannot execute
	}else{
		int wstatus = 0;
		if(!background){
			waitpid(pid, &wstatus, 0);
		}else{
			waitpid(pid, &wstatus, WNOHANG); 
			fprintf(stderr, "process executed in background wait satus = %d\n", wstatus);
		}
		return wstatus;
	}
}


int evaluer_expr_background(Expression *e){
	if(e->type == SIMPLE){
		int ret = execute_simple_command(e->arguments[0], e->arguments, true);
		if(WIFEXITED(ret)){
			fprintf(stderr, "Child exited normally ");
			wait(NULL);
		}
		return ret;
	}
	if(e != NULL){
		int ret = evaluer_expr(e->gauche);
		ret = evaluer_expr_background(e->droite);
		
		return ret;
	}

	fprintf(stderr, "Not implemented yet !\n");
	return 1;
}
			



int evaluer_expr(Expression *e)
{
	if(e == NULL){
		return 1;
	}
	if (e->type == VIDE){
		return 0;
	}

	if(e->type == SIMPLE){
		int ret = execute_simple_command(e->arguments[0], e->arguments,false ); // for simple execute command 
		return ret; // return status of executing the command
	}

	else if(e->type == SEQUENCE){
		int ret = evaluer_expr(e->gauche); // first evaluating left
		ret = evaluer_expr(e->droite); // then evaluate right 
		return ret; //status 
	}
	
	else if(e->type == SEQUENCE_ET){
		int ret = 0;
		if((ret = evaluer_expr(e->gauche)) == 0){ // if the evaluation of left has no error (means returned 0)
			if((ret = evaluer_expr(e->droite)) == 0){ // if the evaluation of the right doesnt return errors
				return ret;
			}
		}
		return ret;
	}

	else if(e->type == SEQUENCE_OU){
		int ret = 0;
		if((ret = evaluer_expr(e->gauche) ) == 0){ // if the first expression is correct no errors returned
			//fprintf(stderr, "ret = %d", ret);
			return ret; // stop execution return success;
		}
		else if( (ret = evaluer_expr(e->droite)) == 0){ // execute this if first expression fail
			return ret;
		}
		return ret;
	}
	else if(e->type == BG){
		int ret = evaluer_expr_background(e->gauche);
		if(e->droite != NULL){
			evaluer_expr(e->droite);
		}else{
			return ret;
		}
	}
	//else if(e->type == REDIRECTION_O){



	fprintf(stderr, "not yet implemented \n");
	return 1;
	
	

}
