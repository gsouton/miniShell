#include "Shell.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int execute_simple_command(char *command, char **args){
	int pid  = fork();
	if(!command){
		int e = execvp(command, args);
		return e;	
	}else{
		return pid;
	}
}



int evaluer_expr(Expression *e)
{
  if (e->type == VIDE)
    return 0;
  if(e->type == SIMPLE){
	int command_to_execute = fork();
	if(!command_to_execute){
		execvp(e->arguments[0], e->arguments);		
	}
	wait(NULL);
	return EXIT_SUCCESS;
  }

  if(e->type == SEQUENCE_ET){
	
 	 
  }
  fprintf(stderr, "not yet implemented \n");
  return 1;
}
