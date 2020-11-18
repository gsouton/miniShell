#include "Evaluation.h"
#include "Shell.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ECHO "echo"
#define CD "cd"
#define SOURCE "source"

static void handler(int signum) {
  /* Take appropriate actions for signal delivery */
  printf("SIGTTIN detected\n");
}
/**
 * @brief enum to know which type of internal command to execute
 *
 */
typedef enum INTERN_CMD { NO_INTERN, _ECHO, _CD, _SOURCE } intern_cmd;

/**
 * @brief Return 1 with a message that the functionality is not implemented
 *
 * @return int
 */
int not_implemented_yet(void) {
  fprintf(stderr, "Not implemented yet !\n");
  return 1;
}

/**
 * @brief check that the condition is true if not
 * 				return 1 and show the last error with perror
 *
 * @param cond condition to check
 * @param msg message to display
 * @return int
 */
int check(int cond, char *msg) {
  if (!cond) {
    perror(msg);
    return 1;
  }
  return 0;
}

/**
 * @brief Return an internal_cmd,
 * 				basically check if a command is an internal command
 * or not if the command is not internal NO_INTERN is returned else see the enum
 * intern_cmd
 *
 * @param cmd command to execute (string of character)
 * @return intern_cmd
 */
intern_cmd is_internal_cmd(char *cmd) {
  if (cmd == NULL) {
    fprintf(stderr, "Wrong paramter passed to is_internal_cmd; cmd == NULL\n");
    return -1;
  } else if (!strcmp(ECHO, cmd)) {
    return _ECHO;
  } else if (!strcmp(CD, cmd)) {
    return _CD;
  } else if (!strcmp(SOURCE, cmd)) {
    return _SOURCE;
  }
  return NO_INTERN;
}

/**
 * @brief Copy of the function echo in bash =>
 * 				function repeat the input given,
 * 				if NULL is given just do an empty line
 *
 * @param arguments Strings to repeat
 * @return int
 */
int echo(char **arguments) {
  int i = 1;
  int w;
  while (arguments[i]) {
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

/**
 * @brief Copy of the source command, execute the command given through a file
 * 				Basically a shell is executed in a child and read from
 * a file instead of the normal tty, so it's reading the command from the file
 * and execute them one by one.
 *
 * @param arguments name of the file to read from (no options implemented)
 * @return int return
 */
int source(char **arguments) {
  int fd = open(arguments[1], O_RDONLY); // open the file
  check(fd >= 0, "open");                // check opening
  int pid = fork();                      // create a child
  if (pid == 0) {                        // child
    dup2(fd, 0);                         // redirect input as the file
    return 0;
    // int status = execl("./Shell", "Shell", NULL); // execute a shell which
    // has as input the file exit(status);
  }
  waitpid(pid, &status, 0); // wait for the child to finish
  return status;
}

/**
 * @brief Execute an internal command (only internal)
 * 				Look at the typeof_cmd and execute the command
 * that correspond to it Of not implemented error message will be displayed
 *
 * @param typeof_cmd enum inter_cmd to know which command to execute 0 is always
 * no command
 * @param arguments the arguments is the command itself with its arguments
 * @param background boolean set to true if the command should be executed in
 * the background
 * @return int return 1 if error else 0
 */
int exec_internal_cmd(intern_cmd typeof_cmd, char **arguments,
                      bool background) {
  if (arguments == NULL || typeof_cmd == NO_INTERN) {
    fprintf(stderr, "Wrong parameter passed to exec_internal_cmd, arguments == "
                    "NULL or type_cmd == NULL\n");
    return -1;
  } else if (typeof_cmd == _ECHO) {
    if (background) {
      int pid = fork();
      if (!pid) {
        exit(echo(arguments)); // exit the program as a child because return is
                               // link to an adress
      }
      return 0;
    }
    return echo(arguments);
    // return not_implemented_yet();
  } else if (typeof_cmd == _CD) {
    return not_implemented_yet();
  } else if (typeof_cmd == _SOURCE) {
    return source(arguments);
  }
  return 1;
}
/**
 * @brief Given the type of redirection through option and a pipe will redirect
 * STDOUT_FILENO and STDIN_FILENO to the right end of the pipe.
 *
 * @param option option that correspond to the type of redirection wanted
 * 							 REDIRECTION_I => redirect the
 * standard input to pipe[0] (and close pipe[1]) REDIRECTION_O => redirect the
 * standard output to pipe[1] (and close pipe[0]) PIPE => redirect both standard
 * output and input
 *
 * @param pipe_fd pipe to redirect to
 */
void manage_pipe_redirection(expr_t option, int *pipe_fd) {
  if ((option == REDIRECTION_I || option == REDIRECTION_O || option == PIPE) &&
      pipe_fd == NULL) {
    fprintf(stderr, "Pipe passed in parameter with a valid option was NULL\n");
  }
  if (option == REDIRECTION_I && pipe_fd != NULL) {
    dup2(pipe_fd[0], 0);
    close(pipe_fd[1]);
  } else if (option == REDIRECTION_O && pipe_fd != NULL) {
    dup2(pipe_fd[1], 1);
    close(pipe_fd[0]);
  } else if (option == PIPE && pipe_fd != NULL) {
    dup2(pipe_fd[0], 0);
    dup2(pipe_fd[1], 1);
  }
}

/**
 * @brief Execute a command using execvp (Cannot execute internal command)
 *
 * 				Create a child process to execute a command given
 * through
 * **args if option set to 0 and pipe to NULL the return value is the pid of the
 * child process Meaning the caller of this function is responsible to wait or
 * not for this child.$ If option and pipe set to correct values, the function
 * will return 0 command will be executed in background and the child will
 * become a zombie if the child process finish its job
 *
 * @param args Command and its arguments to execute
 * @param option option for the redirection of the pipe
 * 							 REDIRECTION_I => redirect STDIN_FILENO
 * to pipe[0] (close pipe[1]) REDIRECTION_O => redirect STDOUT_FILENO to pipe[1]
 * (close pipe[0]) PIPE => redirect both STDOUT_FILENO and STDIN_FILENO
 * @param pipe Pipe (must be initialized before the call of this function)
 * @return int pid of child if option = 0 and pipe = NULL else 0
 */
int exec_command(char **args, expr_t option, int *pipe) {
  int child = fork(); // create a child process
  if (!child) {       // code to execute for the child process

    manage_pipe_redirection(option, pipe);
    execvp(args[0], args);
    fprintf(stderr, "%s : command not found\n", args[0]);
    exit(EXIT_FAILURE); // if execvp fails exit failure
  }
  if (pipe != NULL) { // if pipe is included return 0 we don't wait for children
    return status = 0;
  }
  return child; // return the pid of the child
}

/**
 * @brief Execute a command (Can execute internal commands)
 * 				Use exec_command and exec_internal_cmd to execute
 * the given command Can execute the given command in background if background
 * is set to true (meaning it's to the caller to manage the child zombies etc..)
 *
 * 				Can also execute the command with redirection of to
 * a pipe with the option given The redirection is done in the child process
 * meaning that the redirection of the current process is not modified.
 *
 * 				REDIRECTION_I => redirect STDIN_FILENO to pipe[0]
 * (close pipe[1]) REDIRECTION_O => redirect STDOUT_FILENO to pipe[1] (close
 * pipe[0]) PIPE => redirect STDOUT_FILENO and STDIN_FILENO
 *
 * @param args Command and its arguments to execute
 * @param background boolean: true will execute the command in background, false
 * in the current process
 * @param option To use only with pipe to redirect the the output or input to a
 * pipe
 * @param pipe Pipe to redirect to
 * @return int return 0 if the command didn't failed or executed in background,
 * else return the status error of the command
 */
int execute_command(char **args, bool background, expr_t option, int *pipe) {
  intern_cmd typeof_cmd = is_internal_cmd(args[0]);
  if (typeof_cmd != NO_INTERN) { // if the command is an internal command
    return exec_internal_cmd(typeof_cmd, args, background);
  }

  pid_t pid = exec_command(args, option, pipe); // execute the command

  if (background) { // if background == true we don't wait for children
    return 0;
  }
  int status;                       // status exit of the child
  int w = waitpid(pid, &status, 0); // wait for the child
  check(w > 0, "waitpid");
  return status; // return status of executing the command
}

/**
 * @brief Kill zombies that have been created when call it will wait for only
 * child that terminated
 *
 */
void kill_zombies() {
  int wstatus = 0;
  int w = waitpid(-1, &wstatus, WNOHANG);

  // check(w < 0, "waitpid");

  if (w > 0 && WIFSIGNALED(wstatus)) {
    fprintf(stderr, "process %d was signaled\n", w);
    fprintf(stderr, "%s\n", strsignal(WTERMSIG(wstatus)));
  }
  if (w > 0 && WIFSTOPPED(wstatus)) {
    fprintf(stderr, "process %d was stopped\n", w);
  }

  if (w > 0 && WIFEXITED(wstatus)) {
    fprintf(stderr, "%d, Fini\n", w);
  }
  if (w > 0 && WIFCONTINUED(status)) {
    fprintf(stderr, "%d, Continued\n", w);
  }
}

/**
 * @brief Close both ends of a pipe passed in parameters
 *
 * @param pipefile
 */
void close_pipe(int *fd_p) {
  if (fd_p) {
    close(fd_p[0]);
    close(fd_p[1]);
  }
}

/**
 * @brief Evaluate an expresion that should be executed in the background
 * 				When called with true whenever it will call
 * execute_command with background set to true
 *
 * 				If the expression is not command to execute we
 * evaluate the left part normally and the right part in background until
 * finding the command to execute
 *
 *
 * @param e Expression to evaluate in the background
 * @param background boolean to carry out the execution in background
 * @return int  return 1 in case of error because of expression, else the status
 */
int evaluer_expr_bg(Expression *e, bool background) {
  if (e == NULL) {
    fprintf(stderr, "Expression passed in parameter is NULL\n");
    return 1;
  } else if (e->type == SIMPLE) {
    return execute_command(e->arguments, true, 0, NULL);
  }

  return evaluer_expr(e->gauche);
  return evaluer_expr_bg(e->droite, true);
}

/**
 * @brief Evaluate an expression that has a type REDIRECTION_X
 * 				Will create the file needed and temporary
 * redirect STDOUT_FILENO or STDERR_FILENO or STDIN_FILENO and execute the
 * corresponding command
 *
 * @param e Expression to evaluate
 * @param background boolean set to true if the expression should be executed in
 * a background
 * @param option option to know which redirection to do on the fd
 * 							 voir enum expr_t
 * (REDIRECTION_I, REDIRECTION_O, REDIRECTION_EO, REDIRECTION_A ...)
 * @param fd file descriptor to redirect to
 * @param option_pipe when pipe is needed you can add option with a valid pipe
 * (initialized before) REDIRECTION_I => redirect STDIN_FILENO to pipe[0] (close
 * pipe[1]) REDIRECTION_O => redirect STDOUT_FILENO to pipe[1] (close pipe[0])
 * 										PIPE => redirect
 * both STDOUT_FILENO and STDIN_FILENO
 * @param fd_p NULL if you don't want to redirect to a pipe else a valid pipe
 * already initialized with function pipe()
 * @return int return 0 if success else error code
 */
int evaluer_redirection(Expression *e, bool background, expr_t option, int fd,
                        expr_t option_pipe, int *fd_p) {
  if (e == NULL) {
    fprintf(stderr, "Expression passed in parameter is NULL\n");
    return 1;
  }
  if (e->type == SIMPLE) {
    return execute_command(e->arguments, background, option_pipe, fd_p);
  } else if (e->type == REDIRECTION_I) {
    int file = open(e->arguments[0], O_RDONLY);
    check(file >= 0, "open");
    int save_i = dup(0);
    dup2(file, 0);
    int status = evaluer_redirection(e->gauche, background, e->type, file,
                                     option_pipe, fd_p);
    dup2(save_i, 0);
    close(save_i);
    return status;
  } else if (e->type == REDIRECTION_O) {
    int file = open(e->arguments[0], O_WRONLY + O_TRUNC + O_CREAT, 0666);
    check(file >= 0, "open:");
    int save_o = dup(1);
    dup2(file, 1);
    int status = evaluer_redirection(e->gauche, background, e->type, file,
                                     option_pipe, fd_p);
    dup2(save_o, 1);
    close(save_o);
    close(file);
    return status;
  } else if (e->type == REDIRECTION_A) {
    int file = open(e->arguments[0], O_WRONLY + O_APPEND + O_CREAT, 0666);
    check(file >= 0, "open");
    int save_o = dup(1);
    dup2(file, 1);
    int status = evaluer_redirection(e->gauche, background, e->type, file,
                                     option_pipe, fd_p);
    dup2(save_o, 1);
    close(save_o);
    return status;
  } else if (e->type == REDIRECTION_E) {
    int file = open(e->arguments[0], O_WRONLY + O_CREAT, 0666);
    check(file >= 0, "open");
    int save_e = dup(2);
    dup2(file, 2);
    int status = evaluer_redirection(e->gauche, background, e->type, file,
                                     option_pipe, fd_p);
    dup2(save_e, 2);
    close(save_e);
    return status;
  } else if (e->type == REDIRECTION_EO) {
    int file = open(e->arguments[0], O_WRONLY + O_CREAT, 0666);
    check(file >= 0, "open");
    int save_e = dup(2);
    int save_o = dup(1);
    dup2(file, 1);
    dup2(file, 2);
    int status = evaluer_redirection(e->gauche, background, e->type, file,
                                     option_pipe, fd_p);
    dup2(save_e, 2);
    dup2(save_o, 1);
    close(save_e);
    close(save_o);
    return status;
  }
}

/**
 * @brief Function to evaluate an expression of type PIPE
 * 				Will redirect the standard output or input to
 * the pipe given with the option paramater
 *
 * @param e Expression pipe to evaluate
 * @param option option for pipe redirection
 * 							 REDIRECTION_I => redirect STDIN_FILENO
 * to pipe[0] (close pipe[1]) REDIRECTION_O => redirect STDOUT_FILENO to pipe[1]
 * (close pipe[0]) PIPE => redirect both STDOUT_FILENO and STDIN_FILENO
 *
 * @param fd_p Pipe to redirect to
 * @return int return 0 if success
 */
int evaluer_pipe(Expression *e, expr_t option, int *fd_p) {
  if (e == NULL) {
    // to do
  }
  if (e->type == VIDE) {
    fprintf(stderr, "Expression passed in parameter is NULL\n");
    return 1;
  }
  if (e->type == SIMPLE) {
    // return exec_command(e->arguments, option, fd_p); // doesn't handle
    // internal commands
    return execute_command(e->arguments, true, option,
                           fd_p); // now can execute internal command with pipes
  }
  if (e->type == PIPE) {
    int pipe_fd[2];
    pipe(pipe_fd);
    int nb_children = 1 + evaluer_pipe(e->gauche, REDIRECTION_O, pipe_fd);
    pipe(fd_p);
    close(pipe_fd[1]);
    pipe_fd[1] = fd_p[1];
    nb_children += evaluer_pipe(e->droite, PIPE, pipe_fd);
    return nb_children;
  }
  if (e->type >= REDIRECTION_I) {
    if (option == REDIRECTION_O) {
      int status;
      int save_o = dup(1);
      dup2(fd_p[1], 1);
      status = evaluer_redirection(e, true, e->type, 0, option, fd_p);
      dup2(save_o, 1);
      close(save_o);
      return 0;
    } else if (option == REDIRECTION_I) {
      int save_i = dup(0);
      dup2(fd_p[0], 0);
      evaluer_redirection(e, true, e->type, 0, option, fd_p);
      dup2(save_i, 0);
      close(save_i);
      return 0;
    } else if (option == PIPE) {
      int save_o = dup(1);
      int save_i = dup(0);
      dup2(fd_p[1], 1);
      dup2(fd_p[0], 0);
      evaluer_redirection(e, true, e->type, 0, option, fd_p);
      dup2(save_i, 0);
      dup2(save_o, 1);
      return 0;
    }
  }
}

/**
 * @brief Check if you have multiple pipe for a current context in a tree
 *
 * @param e Expression to look
 * @return int return 1 if another pipe is find else 0
 */
int multiple_pipe(Expression *e) {
  if (e->gauche->type == PIPE) {
    return 1;
  }
  return 0;
}

/**
 * @brief Wait for a given number of children,
 * 				For exemple if you created 3 zombies on purpose (with
 * 2 pipes) At the end of the execution calling this function will kill the
 * zombies
 *
 * @param nb_children number of children to wait for
 * @return int return the last status return of the last children
 */
int wait_children_pipe(int nb_children) {
  int status;
  for (int i = 0; i < nb_children; i++) {
    int w = waitpid(-1, &status, 0);
    // fprintf(stderr, "status %d, retrun wait = %d \n", status, w);
  }
  return status;
}

/**
 * @brief Evaluate an expression of the tree, will look at the given type of the
 * expression and execute the different functions or commands.
 *
 * @param e Expression to evaluate
 * @return int return 0 on success else error code
 */
int evaluer_expr(Expression *e) {
  kill_zombies();

  if (e == NULL) {
    fprintf(stderr, "Expression passed in parameter is NULL\n");
    return 1;
  }

  if (e->type == VIDE) {
    return 0;
  }

  if (e->type == SIMPLE) {
    return execute_command(e->arguments, false, 0, NULL);
  }

  else if (e->type == SEQUENCE) {
    evaluer_expr(e->gauche);        // first evaluating left but
    return evaluer_expr(e->droite); // then evaluate right
    // we only return the value of the last cmd of the sequence ;
  }

  else if (e->type == SEQUENCE_ET) {
    int status = evaluer_expr(e->gauche);
    if (!status) { // if the evaluation of left has no error (means returned 0)
      return evaluer_expr(e->droite); // evaluate the right member
    }
    return status; // else return error of left memeber
  }

  else if (e->type == SEQUENCE_OU) {
    int status = evaluer_expr(e->gauche);
    if (status) { // if left member fails
      return evaluer_expr(e->droite);
    }
    return status;
  }

  else if (e->type == BG) {
    return evaluer_expr_bg(e->gauche, true);
  } else if (e->type >= REDIRECTION_I) {
    return evaluer_redirection(e, false, 0, 0, 0, NULL);
  } else if (e->type == PIPE) {
    int status = 0;
    int fd_p[2];
    int nb_children;

    if (!multiple_pipe(e)) { // if you have only one pipe
      int p = pipe(fd_p);
      check(p >= 0, "pipe");
    }

    nb_children = 1 + evaluer_pipe(e->gauche, REDIRECTION_O, fd_p);
    nb_children += 1 + evaluer_pipe(e->droite, REDIRECTION_I, fd_p);
    close_pipe(fd_p);

    // printf("nb of childrens =  %d\n", nb_children); // debug
    status = wait_children_pipe(nb_children);
    return status;
  }

  return not_implemented_yet();
}
