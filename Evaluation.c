#include "Shell.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


/*
 * Will create a child processus to execute a command 
 * with execvp with the command and args
 * passed in parameters
 * */
int execute_simple_command(char *command, char **args){
	int pid  = fork();
	if(!pid){
		int e = execvp(command, args);
		return e;	
	}else{
		wait(NULL);
		return 1;
	}
}



int evaluer_expr(Expression *e)
{
  if (e->type == VIDE)
    return 0;
  if(e->type == SIMPLE){
	  execute_simple_command(e->arguments[0], e->arguments);
	  return 1;
  }

  /*if(e->type == SEQUENCE_ET){
		  
 	 
  }*/
  fprintf(stderr, "not yet implemented \n");
  return 1;
}
