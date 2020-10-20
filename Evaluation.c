#include "Shell.h"
#include "Evaluation.h"

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
int execute_simple_command(char *command, char **args){
	int pid  = fork();
	if(!pid){
		execvp(command, args);
		exit(EXIT_FAILURE); // if execvp fails it return -1 we could also return the return of execvp but this line would execute only if execvp cannot execute
	}else{
		int wstatus;
		wait(&wstatus);
		return wstatus;
	}
}



int evaluer_expr(Expression *e)
{
	if (e->type == VIDE)
		return 0;

	if(e->type == SIMPLE){
		int ret = execute_simple_command(e->arguments[0], e->arguments);
		return ret;
	}

	else if(e->type == SEQUENCE){
		int ret = evaluer_expr(e->gauche);
		ret = evaluer_expr(e->droite);
		return ret;
	}
	
	else if(e->type == SEQUENCE_ET){
		int ret = 0;
		if((ret = evaluer_expr(e->gauche)) == 0){
			if((ret = evaluer_expr(e->droite)) == 0){
				return 0;
			}
		}
		return ret;
	}

	else if(e->type == SEQUENCE_OU){
		int ret = 0;
		if((ret = evaluer_expr(e->gauche) ) == 0){
			//fprintf(stderr, "ret = %d", ret);
			return ret;
		}
		else if( (ret = evaluer_expr(e->droite)) == 0){
			return ret;
		}
		return ret;
	}else{

	fprintf(stderr, "not yet implemented \n");
	return 1;
	}

}
