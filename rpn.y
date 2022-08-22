%{
  #include <stdio.h>
  #include <math.h>
  int yylex(void);
  void yyerror(char const *);
%}

%union {
  double d;
}

%token <d> NUM
%type <d> exp

%%

input:
     %empty
     | input line
     ;

line:
    '\n'
    | exp '\n'        { printf("= %.10g\n", $1); }
    ;

exp:
   NUM
   | exp exp '+'   { $$ = $1 + $2; }
   | exp exp '-'   { $$ = $1 - $2; }
   | exp exp '^'   { $$ = pow($1, $2); }
   | exp 'n'       { $$ = -$1; }
   ;

%%

int main(void) {
  return yyparse();
}

void yyerror(char const *s) {
  fprintf(stderr, "%s\n", s);
}
