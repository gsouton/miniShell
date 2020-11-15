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
#include <stdarg.h>
#include <signal.h>

#define ECHO "echo"
#define CD "cd"
#define SOURCE "source"

typedef enum INTERN_CMD{
	NO_INTERN,
	_ECHO,
	_CD,
	_SOURCE
}intern_cmd;

typedef enum TYPE_OF_REDIRECTION{
	NO_REDIRECTION,
	INPUT,  // Redirection entree
  OUTPUT = INPUT << 1,  // Redirection sortie standard
  APPEND = OUTPUT << 1,  // Redirection sortie standard, mode append
  ERROR = APPEND << 1,  // Redirection sortie erreur
  ERROR_N_OUT = ERROR << 1
}type_of_redirection;

typedef struct REDIRECTION_OBJ{
	int fd;
	type_of_redirection type;
}obj_rdr;

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
		int i = 1;
		int w;
		while(arguments[i]){
			w = write(1, arguments[i], strlen(arguments[i]));
			check(w > 0, "write ");
			w = write(1, " ", sizeof(char));
			check(w > 0, "write ");

			i++;
		}
		w = write(1, "\n", sizeof(char));
		check(w > 0, "write");

		return 0;
}

int source(char **arguments){
	int fd = open(arguments[1], O_RDONLY); // open the file
	check(fd >= 0, "open"); // check opening
	int pid = fork(); // create a child
	if(pid == 0){ //child
		dup2(fd, 0); //redirect input as the file
		return 0;
		//int status = execl("./Shell", "Shell", NULL); // execute a shell which has as input the file
		//exit(status);
	}
	waitpid(pid, &status, 0);
	return status;
}


/*
 * Will execute an internal command
 */
int exec_internal_cmd(intern_cmd typeof_cmd, char **arguments, bool background){
	if(arguments == NULL || typeof_cmd == NO_INTERN){
		//TO DO;
	}
	else if(typeof_cmd == _ECHO){
		if(background){
			int pid = fork();
			if(!pid){
				exit(echo(arguments)); // exit the program as a child because return is link to an adress
			}
			return 0;
		}
		return echo(arguments);
		//return not_implemented_yet();
	}
	else if(typeof_cmd == _CD){
		return not_implemented_yet();
	}
	else if(typeof_cmd == _SOURCE){
		return source(arguments);
	}
	return 1;

}

void redirection(expr_t option, int fd){
	switch (option) {
		case REDIRECTION_I:
			dup2(fd, 0);
			break;
		case REDIRECTION_O:
			dup2(fd, 1);
		default:
			break;
	}
}

/* <summary>
*		Will create a child process
*		The child will execute the command with execvp and then return
*    the pid
*   	The caller as to deal with the pid and waiting
*	</summary>
*/
int exec_command(char **args, expr_t option, int fd){
 int child = fork(); // create a child process

 if(!child){ // code to execute for the child process
	 //redirection(option, fd);
	 execvp(args[0], args);
	 fprintf(stderr, "%s : command not found\n", args[0]);
	 exit(EXIT_FAILURE); // if execvp fails exit failure
 }
 return child; // return the pid of the child

}

int execute_command(char **args, bool background, expr_t option, int fd){
	intern_cmd typeof_cmd = is_internal_cmd(args[0]);
	if(typeof_cmd != NO_INTERN){ //if the command is an internal command
		return exec_internal_cmd(typeof_cmd, args	, background);
	}

	pid_t pid = exec_command(args, option, fd); // execute the command

	if(background){ // if background == true we don't wait for children
		return 0;
	}
	int status; // status exit of the child
	int w = waitpid(pid, &status, 0); //wait for the child
	check(w > 0, "waitpid");
	return status; // return status of executing the command


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
	//check(w < 0, "waitpid");
	if(w >= 0){
		fprintf(stderr, "process %d\n", w);
	}
	if(w > 0 && WIFSIGNALED(wstatus)){
		fprintf(stderr, "process %d was signaled\n", w);
		fprintf(stderr, "%s\n", strsignal(WTERMSIG(wstatus)));
	}
	if(w > 0 && WIFSTOPPED(wstatus)){
		fprintf(stderr, "process %d was stopped\n");
	}

	if(WIFEXITED(wstatus)){
		fprintf(stderr, "%d, Fini\n", w);
	}

}



/*
 * Parameters =>
 *	@int *pipefile : reference to the pipe that you want to close
 *
 * <summary>
 *		Will close both extremity of the pipe
 *		So make the pipe not usable anymore neither in writing or reading
 * </summary>
 */
void close_pipe(int *pipefile){
	if(pipefile){
		close(pipefile[0]);
		close(pipefile[1]);
	}
}


int evaluer_expr_bg(Expression *e, bool background){
	if(e == NULL){
		fprintf(stderr, "Expression passed in parameter is NULL\n");
		return 1;
	}
	else if(e->type == SIMPLE){
		return execute_command(e->arguments, true, 0, 0);
	}

	return evaluer_expr(e->gauche);
	return evaluer_expr_bg(e->droite, true);
}

int evaluer_redirection(Expression *e, bool background, expr_t option, int fd){
	if(e == NULL){
		fprintf(stderr, "Expression passed in parameter is NULL\n");
		return 1;
	}
	if(e->type == SIMPLE){
		return execute_command(e->arguments, background, option, fd);
	}
	else if(e->type == REDIRECTION_I){
		int file = open(e->arguments[0], O_RDONLY);
		check(file >= 0, "open");
		int save_i = dup(0);
		dup2(file, 0);
		int status = evaluer_redirection(e->gauche, background, e->type, file);
		dup2(save_i, 0);
		return status;
	}
	else if(e->type == REDIRECTION_O){
		int file = open(e->arguments[0], O_WRONLY + O_TRUNC + O_CREAT, 0666);
		check(file >= 0, "open:");
		int save_o = dup(1);
		dup2(file, 1);
		int status = evaluer_redirection(e->gauche, background, e->type, file);
		dup2(save_o, 1);
		return status;
	}
	else if(e->type == REDIRECTION_A){
		int file = open(e->arguments[0], O_WRONLY + O_APPEND + O_CREAT, 0666);
		check(file >= 0, "open");
		int save_o = dup(1);
		dup2(file, 1);
		int status = evaluer_redirection(e->gauche, background, e->type, file);
		dup2(save_o, 1);
		return status;
	}
	else if(e->type == REDIRECTION_E){
		int file = open(e->arguments[0], O_WRONLY + O_CREAT, 0666);
		check(file >= 0, "open");
		int save_e = dup(2);
		dup2(file, 2);
		int status = evaluer_redirection(e->gauche, background, e->type, file);
		dup2(save_e, 2);
		return status;
	}
	else if(e->type == REDIRECTION_EO){
		int file = open(e->arguments[0], O_WRONLY + O_CREAT, 0666);
		check(file >= 0, "open");
		int save_e = dup(2);
		int save_o = dup(1);
		dup2(file, 1);
		dup2(file, 2);
		int status = evaluer_redirection(e->gauche, background, e->type, file);
		dup2(save_e, 2);
		dup2(save_o, 1);
		return status;
	}

}


int evaluer_expr(Expression *e){
	kill_zombies();

	if(e == NULL){
		fprintf(stderr, "Expression passed in parameter is NULL\n");
		return 1;
	}

	if (e->type == VIDE){
		return 0;
	}

	if(e->type == SIMPLE){
		return execute_command(e->arguments, false, 0, 0);
	}

	else if(e->type == SEQUENCE){
		evaluer_expr(e->gauche); // first evaluating left but
		return evaluer_expr(e->droite); // then evaluate right
		//we only return the value of the last cmd of the sequence ;
	}

	else if(e->type == SEQUENCE_ET){
		int status = evaluer_expr(e->gauche);
		if(!status){ // if the evaluation of left has no error (means returned 0)
			return evaluer_expr(e->droite); //evaluate the right member
		}
		return status; //else return error of left memeber
	}

	else if(e->type == SEQUENCE_OU){
		int status = evaluer_expr(e->gauche);
		if(status){ // if left member fails
			return evaluer_expr(e->droite);
		}
		return status;
	}

	else if(e->type == BG){
		return evaluer_expr_bg(e->gauche, true);
	}
	else if(e->type >= REDIRECTION_I){
		return evaluer_redirection(e, false, 0, 0);
	}
	else if(e->type == PIPE){
		

	}

	return not_implemented_yet();


}
