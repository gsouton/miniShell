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
		int e = execvp(command, args);
		return e; // in case of invalid command zombie will be created
	}else{
		int wstatus;
		int w = waitpid(pid, &wstatus, 0 );
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
		evaluer_expr(e->gauche);
		evaluer_expr(e->droite);
		return 0;
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
		if((ret = evaluer_expr(e->gauche) == 0)){
			return ret;
		}
		else if( (ret = evaluer_expr(e->droite)) == 0){
			return ret;
		}
		return ret;
	}



	fprintf(stderr, "not yet implemented \n");
	return 1;
}
