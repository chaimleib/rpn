%{
  #include <ctype.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  int yylex(void);
  void yyerror(char const *);

  #define YYSTYPE double
%}

%token NUM

%%

input:
     // %empty
     | input line
     ;

line:
    '\n'
    | exp '\n'        { printf("%.10g\n", $1); }
    ;

exp:
   NUM
   | exp exp '+'   { $$ = $1 + $2; }
   | exp exp '-'   { $$ = $1 + $2; }
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

int yylex(void) {
  int c = getchar();
  while (c == ' ' || c == '\t')
    c = getchar();

  if (c == '.' || isdigit(c)) {
    ungetc(c, stdin);
    if (scanf("%lf", &yylval) != 1)
      abort();
    return NUM;
  } else if (c == EOF) {
    return YYEOF;
  } else {
    return c;
  }
}
