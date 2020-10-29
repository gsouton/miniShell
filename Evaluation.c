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

/*typedef enum OPTIONS {
  NO_OPTIONS,
  INPUT_RE,
  OUTPUT_RE = INPUT_RE << 1,
  APPEND_RE = OUTPUT_RE << 1,
  ERROR_RE = APPEND_RE << 1,
  ERROR_AND_OUT = ERROR_RE << 1
  }opt_t;*/


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

void manage_redirection(opt_t options, int fd, int *pipe_fd){
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
	if(options & PIPE_O){
		close(pipe_fd[0]);
	}
	if(options & PIPE_I){
		close(pipe_fd[1]);
	}
}

void close_pipe(int *pipe_fd){
	if(pipe_fd){
		close(pipe_fd[0]);
		close(pipe_fd[1]);
	}
}


/*
 * Will create a child processus to execute a command
 * with execvp with the command and args
 * passed in parameters
 * */
int execute_command(char *command, char **args, bool background, opt_t options, int fd, int *pipe_fd){
	int pid  = fork();

	if(!pid){
		manage_redirection(options, fd, pipe_fd);
		execvp(command, args);
		exit(EXIT_FAILURE); // if execvp fails it return -1 we could also return the return of execvp but this line would execute only if execvp cannot execute
	}else{
		int wstatus = 0;
		if(!background){
			waitpid(pid, &wstatus, 0);

		}else{
			fprintf(stderr, "Process executed in background %d\n",pid);
		}
		return wstatus;
	}
}

int exec_command(char *command, char **args, opt_t options, int fd, int *pipe_fd){
	int pid = fork();

	if(!pid){
		manage_redirection(options, fd, pipe_fd);
		execvp(command, args);
		exit(EXIT_FAILURE);
	}
	return pid;

}

int evaluer_expr_redir(Expression *e, bool background, int fd, opt_t options){
	if(e->type == SIMPLE){
		int ret = execute_command(e->arguments[0], e->arguments, background, options, fd, NULL);
		return ret;
	}
	fprintf(stderr, "Not implemented yet ! \n");
	return 1;
}



int evaluer_redirection(Expression *e, bool background){
	if(e->type == REDIRECTION_I){
		int fd = open(e->arguments[0], O_RDONLY);
		if(check(fd> 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, background, fd, INPUT_RE);
		close(fd);
		return ret;

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
		int fd = open(e->arguments[0], O_WRONLY + O_APPEND + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, background, fd, APPEND_RE);
		close(fd);
		return ret;

	}
	else if(e->type == REDIRECTION_E){
		int fd = open(e->arguments[0], O_WRONLY + O_TRUNC + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, background, fd, ERROR_RE);
		close(fd);
		return ret;

	}
	else if(e->type == REDIRECTION_EO){
		int fd = open(e->arguments[0], O_WRONLY + O_TRUNC + O_CREAT, 0666);
		if(check(fd > 0, "open")){
			return 1;
		}
		int ret = evaluer_expr_redir(e->gauche, background,fd, OUTPUT_RE + ERROR_RE);
		close(fd);
		return ret;

	}
	return 1;
}







int evaluer_expr_background(Expression *e){
	if(e->type == SIMPLE){
		int ret = execute_command(e->arguments[0], e->arguments, true, NO_OPTIONS, 0, NULL);
		//int ret = exec_command(e->arguments[0], e->arguments, NO_OPTIONS, 0, NULL);
		return ret;
	}
	else if(e->type == SEQUENCE || e->type == SEQUENCE_ET){
		int ret =  evaluer_expr(e->gauche);
		ret =  evaluer_expr_background(e->droite);
		return ret;
	}

	else if(e->type == REDIRECTION_O ||
			e->type == REDIRECTION_I ||
			e->type == REDIRECTION_A ||
			e->type == REDIRECTION_E ||
			e->type == REDIRECTION_EO){
		return evaluer_redirection(e, true);
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


int manage_pipe(Expression *e, bool background, opt_t option, int pipe_to_redirect, int *pipe_fd){

	if(e->type == SIMPLE){
		return exec_command(e->arguments[0], e->arguments, option, pipe_to_redirect, pipe_fd);
	}else{

		return evaluer_expr(e);
	}


}


int evaluer_expr(Expression *e){
	if(e == NULL){
		return 1;
	}
	kill_zombies();

	if (e->type == VIDE){
		return 0;
	}

	if(e->type == SIMPLE){
		//int ret = execute_command(e->arguments[0], e->arguments,false, NO_OPTIONS, 0, NULL); // for simple execute command
		int ret = exec_command(e->arguments[0], e->arguments, NO_OPTIONS, 0, NULL);
		int status;
		int w = waitpid(ret, &status, 0);
		check(w > 0, "waitpid");
		
		return status; // return status of executing the command
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
	else if(e->type == REDIRECTION_O ||
			e->type == REDIRECTION_I ||
			e->type == REDIRECTION_A ||
			e->type == REDIRECTION_E ||
			e->type == REDIRECTION_EO){
		return evaluer_redirection(e, false);
	}
	else if(e->type == PIPE){
		int pipe_fd[2];
		int create_pipe = pipe(pipe_fd);
		check(create_pipe == 0, "pipe");
		int status1, status2;


		int left_command = manage_pipe(e->gauche, false, OUTPUT_RE + PIPE_O, pipe_fd[1], pipe_fd);
		int right_command = manage_pipe(e->droite, false, INPUT_RE + PIPE_I, pipe_fd[0], pipe_fd);
		close_pipe(pipe_fd);
		waitpid(left_command, &status1, 0);
		waitpid(right_command, &status2, 0);
		if(status1 != 0){
			return status1;
		}
		return status2;

	}


	fprintf(stderr, "not yet implemented \n");
	return 1;



}
