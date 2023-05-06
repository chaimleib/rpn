%{
#include <stdio.h>
#include "ast.h"

int yylex(void);
void yyerror(char const *);
%}

%union {
  double d;
  struct _node* n;
  int i;
}

%token <d> NUM
%token <i> '+' '-' '^' 'n'
%nterm <n> exp op

%%

input
  : /* empty */
  | input line
  ;

line
  : '\n'
  | exp '\n'      { printf("%s\n", ast2str($1)); }
  ;

exp
  : NUM           { $$ = mknod(NUM, $1, NULL, NULL, NULL); }
  | op            { $$ = $1; }
  ;

op
  : exp exp '+'   { $$ = mknod('+', 0, $1, $2, NULL); }
  | exp exp '-'   { $$ = mknod('-', 0, $1, $2, NULL); }
  | exp exp '^'   { $$ = mknod('^', 0, $1, $2, NULL); }
  | exp 'n'       { $$ = mknod('n', 0, $1, NULL, NULL); }
  ;

%%

int main(void) {
  return yyparse();
}

void yyerror(char const *s) {
  fprintf(stderr, "%s\n", s);
}
