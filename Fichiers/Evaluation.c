#include "Shell.h"
#include "Evaluation.h"

#include <stdio.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

int evaluer_expr(Expression *e)
{
  if (e->type == VIDE)
    return 0;
  fprintf(stderr, "not yet implemented \n");
  return 1;
}
