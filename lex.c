#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "rpn.tab.h"

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
