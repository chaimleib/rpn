%{
  #include <stdio.h>
  #include <stdlib.h>
  #include "rpn.tab.h"

  int getDouble(void);
%}

%option nounput
%option noinput

%%

[ \t]+                     ;
-?[1-9][0-9]*(\.[0-9]*)?   return getDouble();
-?0?\.[0.9]*               return getDouble();
.                          return yytext[0];
"\n"                       return yytext[0];

%%

int yywrap() {
  return 1;
}

int getDouble() {
  yylval.d = atof(yytext);
  return NUM;
}
