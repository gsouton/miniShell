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

#define ECHO "echo"
#define CD "cd"
#define SOURCE "source"

typedef enum INTERN_CMD{
	NO_INTERN,
	_ECHO,
	_CD,
	_SOURCE
}intern_cmd;

int not_implemented_yet(void){
		fprintf(stderr, "Not implemented yet !\n");
		return 1;
}

/*
 * If a cond is not verified do a perror on the last error
 */
int check(int cond, char *msg){
	if(!cond){
		perror(msg);
		return 1;
	}
	return 0;
}

/*
 * check for a given command as a char* if it is an internal command
 *
 */
intern_cmd is_internal_cmd(char *cmd){
	if(cmd == NULL){
		//TO DO
	}
	else if(!strcmp(ECHO, cmd)){
		return _ECHO;
	}
	else if(!strcmp(CD, cmd)){
		return _CD;
	}
	else if(!strcmp(SOURCE, cmd)){
		return _SOURCE;
	}
	return NO_INTERN;
}

int echo(char **arguments){
	if(arguments[1] == NULL){
		char buffer; //buffer of 1 character
		int r = 0; // number of character read
		while((r = read(0, &buffer, sizeof(buffer)) != 0)){
			int w = write(1, &buffer, r);
		}
		return 0;
	}else{
		int i = 1;
		while(arguments[i]){
			write(1, arguments[i], strlen(arguments[i]));
			write(1, " ", sizeof(char));

			i++;
		}
		write(1, "\n", sizeof(char));
		return 0;
	}
}


/*
 * Will execute an internal command
 */
int exec_internal_cmd(intern_cmd typeof_cmd, char **arguments){
	if(arguments == NULL || typeof_cmd == NO_INTERN){
		//TO DO;
	}
	else if(typeof_cmd == _ECHO){
		return echo(arguments);
		//return not_implemented_yet();
	}
	else if(typeof_cmd == _CD){
		return not_implemented_yet();
	}
	else if(typeof_cmd == _SOURCE){
		return not_implemented_yet();
	}
	return 1;

}



/*
 * <summary>
 *		Every time this function is called it will call waitpid to wait
 *		any child process but no suspend the main process (OPTION WNOHANG)
 *		If the call of waitpid with no hang doesnt return 0 then the process changed state and then exited
 *		we then print that the process is finished
 *</summary>
 */
void kill_zombies(){
	int wstatus;
	int w = waitpid(-1, &wstatus, WNOHANG);
	//check( w >= 0, "waitpid");

	if(w > 0){
		//waitpid(w, &wstatus, 0);
		fprintf(stderr, "%d, Fini\n", w);
	}
}

/*
 * Parameters =>
 *	@opt_t options : options that describe the type of redirection
 *	@int fd : file descriptor to redirect or to redirect to
 *	@int *pipe_fd: if not null and options specified it will close the part of the pipe that isn't use in the redirection
 *
 * <summary>
 *		Will look at the options
 *		and redirect either STDOUT, STDERR, or both or STDIN to be fd
 *		if pipe is specified not null and with the right options it will close also the
 *		part of the pipe that is not used
 *
 *</summary>
 */
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
		dup2(pipe_fd[1], 1);
		close(pipe_fd[0]);
	}
	if(options & PIPE_I){
		dup2(pipe_fd[0], 0);
		close(pipe_fd[1]);
	}
}

/*
 * Parameters =>
 *	@int *pipe_fd : reference to the pipe that you want to close
 *
 * <summary>
 *		Will close both extremity of the pipe
 *		So make the pipe not usable anymore neither in writing or reading
 * </summary>
 */
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

/*
 * Parameters =>
 *	@char *command : string of the command to execute
 *	@char **args : arguments of the command
 *	@opt_t options : Check Evaluation.h mainly options of redirections and pipe, when no options put 0
 *	@int fd : file descriptor that will be used for redirection specified by options if no options fd wont be used
 *	@int *pipe_fd : pipe to precise when want to execute a command and redirect it to a pipe options must be specified NULL when no pipe is needed
 *
 * <summary>
 *		Will create a child process
 *		this child process will do some redirection if needed
 *		then execute the command if the command fail it will exit
 *		The father just return the pid of his child
 *	</summary>
 */
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



/*
 * Parameters =>
 *	@Expression *e : Expression of type redirection to evaluate
 *	@bool background : bool set to true if the following instructions should be executed in background else to false
 *
 * <summary>
 *		Actually this function only take in parameters expression of type redirection
 *		REDIRECTION_O, REDIRECTION_E, REDIRECTION_A etc..
 *		for each case it will open the file given in arguments with the corrects permission, reading, writing, append etc...
 *		Then it will call the function evaluer_expr_redir with the right filedescriptor and options in parameters
 *
 *		This function is only call on the left branch because there is nothing to execute at the right of a redirection
 *		it will store the return of this function
 *		close the file
 *		and return the status
 * </summary>
 */
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
		//int ret = execute_command(e->arguments[0], e->arguments, true, NO_OPTIONS, 0, NULL);
		int ret = exec_command(e->arguments[0], e->arguments, NO_OPTIONS, 0, NULL);
		int status;
		int w = waitpid(ret, &status, WNOHANG);
		check(w > 0, "waitpid");

		return status;
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


int manage_pipe(Expression *e, bool background, opt_t options, int *pipe_fd){

	if(e->type == SIMPLE){
		return exec_command(e->arguments[0], e->arguments, options, 0, pipe_fd);
	}

	else if(e->type >= REDIRECTION_I){

	}

	return evaluer_expr(e);



}


int evaluer_expr(Expression *e){
	kill_zombies();

	if(e == NULL){
		return 1;
	}

	if (e->type == VIDE){
		return 0;
	}

	if(e->type == SIMPLE){
		intern_cmd typeof_cmd = is_internal_cmd(e->arguments[0]);
		if(typeof_cmd != NO_INTERN){ //if the command is an internal command
			return exec_internal_cmd(typeof_cmd, e->arguments);
		}

		int pid = exec_command(e->arguments[0], e->arguments, NO_OPTIONS, 0, NULL); // exec the command
		int status; // status exit of the child
		int w = waitpid(pid, &status, 0); //wait for the child
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
	else if(e->type >= REDIRECTION_I){
		return evaluer_redirection(e, false);
	}

	else if(e->type == PIPE){
		int pipe_fd[2];
		int create_pipe = pipe(pipe_fd);
		check(create_pipe == 0, "pipe");
		int status1, status2;


		int left_command = manage_pipe(e->gauche, false, PIPE_O, pipe_fd);
		int right_command = manage_pipe(e->droite, false, PIPE_I, pipe_fd);
		close_pipe(pipe_fd);
		waitpid(left_command, &status1, 0);
		waitpid(right_command, &status2, 0);
		if(status1 != 0){
			return status1;
		}
		return status2;

	}

	return not_implemented_yet();



}

int check_for_intern(Expression *e){
	if(e->type != SIMPLE){
		fprintf(stderr, "Wrong ue of check for intern\n");
		return 1;
	}
	if(! strcmp("echo", e->arguments[0])){
		//fprintf(stderr, "internal command to code !! <echo>\n");
		return echo(e->arguments);
	}
	else if(! strcmp("cd", e->arguments[0])){
		fprintf(stderr, "internal command to code!! <cd> \n");
		return 1;
	}
	return 1;
}
