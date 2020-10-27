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

typedef enum OPTIONS {
	NO_OPTIONS,
	INPUT_RE,
	OUTPUT_RE = INPUT_RE << 1,
	APPEND_RE = OUTPUT_RE << 1,
	ERROR_RE = APPEND_RE << 1,
	ERROR_AND_OUT = ERROR_RE << 1,
	SIZE = 6
}opt_t;


int check(int cond, char *msg){
	if(!cond){
		perror(msg);
		return 1;
	}
	return 0;
}

void kill_zombies(){
	int wstatus;
	int w = waitpid(-1, &wstatus, WNOHANG);
	if(w > 0){
		waitpid(w, &wstatus, 0);
		fprintf(stderr, "%d, Fini\n", w);
	}
}

void manage_redirection(opt_t options, int fd){
	if(options == 0){
		return;
	}
	if(options & INPUT_RE){
		dup2(fd, 0);
	}
	if(options & OUTPUT_RE){
		dup2(fd, 1);
	}
	if(options & APPEND_RE){
		dup2(fd, 1);
	}
	if(options & ERROR_RE){
		dup2(fd, 2);
	}
	if(options & ERROR_AND_OUT){
		dup2(fd, 1);
		dup2(fd, 2);
	}
}


/*
 * Will create a child processus to execute a command
 * with execvp with the command and args
 * passed in parameters
 * */
int execute_command(char *command, char **args, bool background, opt_t options, int fd){
	int pid  = fork();

	if(!pid){
		manage_redirection(options, fd);
		execvp(command, args);
		exit(EXIT_FAILURE); // if execvp fails it return -1 we could also return the return of execvp but this line would execute only if execvp cannot execute
	}else{
		int wstatus = 0;
		if(!background){
			waitpid(pid, &wstatus, 0);
		}else{
			//int w = waitpid(pid, &wstatus, WNOHANG);
			//check(w >= 0, "waitpid");
			fprintf(stderr, "Process executed in background %d\n",pid);
		}
		return wstatus;
	}
}


int evaluer_expr_background(Expression *e){
	if(e->type == SIMPLE){
		int ret = execute_command(e->arguments[0], e->arguments, true, NO_OPTIONS, 0);
		return ret;
	}
	else if(e->type == SEQUENCE){
		int ret =  evaluer_expr(e->gauche);
		ret =  evaluer_expr_background(e->droite);
		return ret;
	}


	if(e->gauche != NULL){
		return evaluer_expr(e);
	}
	if(e->droite != NULL){
		return evaluer_expr_background(e->droite);
	}

	fprintf(stderr, "Not implemented yet !\n");
	return 1;
}

int evaluer_expr_redir(Expression *e, bool background, int fd, opt_t options){
	if(e->type == SIMPLE){
		int ret = execute_command(e->arguments[0], e->arguments, background, options, fd);
		return ret;
	}
	fprintf(stderr, "Not implemented yet ! \n");
	return 1;
}


int evaluer_redirection(Expression *e, bool background){
	if(e->type == REDIRECTION_I){

	}
	else if(e->type == REDIRECTION_O){
		int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, background, fd, OUTPUT_RE);
		close(fd);
		return ret;

	}
	else if(e->type == REDIRECTION_A){

	}
	else if(e->type == REDIRECTION_E){

	}
	else if(e->type == REDIRECTION_EO){

	}
	return 1;
}



int evaluer_expr(Expression *e)
{
	if(e == NULL){
		return 1;
	}
	kill_zombies();

	if (e->type == VIDE){
		return 0;
	}

	if(e->type == SIMPLE){
		int ret = execute_command(e->arguments[0], e->arguments,false, NO_OPTIONS, 0 ); // for simple execute command
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
		return ret;

	}
	else if(e->type == REDIRECTION_O){
		int fd = open(e->arguments[0], O_WRONLY | O_CREAT | O_TRUNC, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, false, fd, OUTPUT_RE);
		close(fd);
		return ret;
	}
	else if(e->type == REDIRECTION_I){
		int fd = open(e->arguments[0], O_RDONLY);
		if(check(fd> 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, false, fd, INPUT_RE);
		close(fd);
		return ret;
	}
	else if(e->type == REDIRECTION_A){
		int fd = open(e->arguments[0], O_WRONLY + O_APPEND + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, false, fd, APPEND_RE);
		close(fd);
		return ret;
	}
	else if(e->type == REDIRECTION_E){
		int fd = open(e->arguments[0], O_WRONLY + O_TRUNC + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, false, fd, ERROR_RE);
		close(fd);
		return ret;
	}
	else if(e->type == REDIRECTION_EO){
		int fd = open(e->arguments[0], O_WRONLY, O_TRUNC + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, true,fd, ERROR_AND_OUT);
		close(fd);
		return ret;
	}



	fprintf(stderr, "not yet implemented \n");
	return 1;



}
