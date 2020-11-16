#ifndef ANALYSE
#define ANALYSE

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define NB_ARGS 50
#define TAILLE_ID 500

typedef enum expr_t
{
  VIDE,           // 0 Commande vide
  SIMPLE,         // 1 Commande simple
  SEQUENCE,       // 2 S�quence (;)
  SEQUENCE_ET,    // 3 S�quence conditionnelle (&&)
  SEQUENCE_OU,    // 4 S�quence conditionnelle (||)
  BG,             // 5 Tache en arriere plan
  PIPE,           // 6 Pipe
  REDIRECTION_I,  // 7 Redirection entree               0111
  REDIRECTION_O,  // 8 Redirection sortie standard      1000
  REDIRECTION_A,  // 9 Redirection sortie standard, mode append
  REDIRECTION_E,  // 10 Redirection sortie erreur
  REDIRECTION_EO, // 11 Redirection sorties erreur et standard
} expr_t;

typedef struct Expression
{
  expr_t type;
  struct Expression *gauche;
  struct Expression *droite;
  char **arguments;
} Expression;

extern int yyparse(void);
Expression *ConstruireNoeud(expr_t, Expression *, Expression *, char **);
char **AjouterArg(char **, char *);
char **InitialiserListeArguments(void);
int LongueurListe(char **);
void EndOfFile(void);

void yyerror(char *s);
extern Expression *ExpressionAnalysee;
extern int status;

#endif /* ANALYSE */
