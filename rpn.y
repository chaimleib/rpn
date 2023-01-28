%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
int yylex(void);
void yyerror(char const *);

typedef struct _node {
  int type;
  double d;
  struct _node *arg[3];
} node;

node *root;

node* mknod(int type, double d, node *a, node *b, node *c) {
  node *rv = (node *) malloc(sizeof(node));
  rv->type = type;
  rv->d = d;
  rv->arg[0] = a;
  rv->arg[1] = b;
  rv->arg[2] = c;
  return rv;
}

char *ast2str(node *);
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
  : %empty
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
  : exp exp '+'   { $$ = mknod($3, 0, $1, $2, NULL); }
  | exp exp '-'   { $$ = mknod($3, 0, $1, $2, NULL); }
  | exp exp '^'   { $$ = mknod($3, 0, $1, $2, NULL); }
  | exp 'n'       { $$ = mknod($2, 0, $1, NULL, NULL); }
  ;

%%

int main(void) {
  return yyparse();
}

void yyerror(char const *s) {
  fprintf(stderr, "%s\n", s);
}

#define MAX_ALLOC (16*1024)

typedef struct {
  size_t cap;
  size_t i;
  node **store;
} stack;

stack* mkstack() {
  stack *self = calloc(sizeof(stack), 1);
  if (self == NULL) {
    fprintf(stderr, "error: mkstack(): self: nomem\n");
    abort();
    return NULL;
  }
  self->cap = 16;
  self->i = 0;
  self->store = calloc(sizeof(node *), self->cap);
  if (self->store == NULL) {
    fprintf(stderr, "error: mkstack(): store: nomem\n");
    abort();
    return NULL;
  }
  return self;
}

void free_stack(stack *self) {
  free(self->store);
  free(self);
}

void push(stack *self, node *n) {
  size_t newcap = self->cap;
  while (self->i >= newcap) newcap *= 2;
  if (newcap != self->cap) {
    if (sizeof(node*) * newcap > MAX_ALLOC) {
      fprintf(stderr, "error: push(): exceeded MAX_ALLOC\n");
      abort();
    }
    self->store = realloc(self->store, sizeof(node *) * newcap);
    if (self->store == NULL) {
      fprintf(stderr, "error: push(): nomem\n");
      abort();
    }
    memset(&self->store[self->cap], '\0', sizeof(node)*(newcap - self->cap));
    self->cap = newcap;
  }
  self->store[self->i] = n;
  self->i++;
}

node* pop(stack *self) {
  if (self->i == 0) return NULL;
  self->store[self->i--] = NULL;
  return self->store[self->i];
}

char *ast2str(node *root) {
  char *result, *tmp, *indent;
  int resultCap = 64, tmpCap = 64, indentCap = 64;
  node *n;
  stack *s;

  result = calloc(sizeof(char), resultCap);
  if (result == NULL) {
    fprintf(stderr, "error: ast2str(): nomem\n");
    abort();
  }
  tmp = calloc(sizeof(char), tmpCap);
  if (tmp == NULL) {
    fprintf(stderr, "error: ast2str(): nomem\n");
    abort();
  }
  indent = calloc(sizeof(char), indentCap);
  if (indent == NULL) {
    fprintf(stderr, "error: ast2str(): nomem\n");
    abort();
  }

  s = mkstack();
  push(s, root);

  while (n = pop(s)) {
    switch (n->type) {
    case NUM:
      sprintf(tmp, "%sNUM %0.2f\n", indent, n->d);
      sprintf(result, "%s%s", result, tmp);
    }
  }
  free(indent);
  free(tmp);
  tmp = result;
  result = strdup(tmp);
  free(tmp);
  free_stack(s);

  printf("out: %s", result);
  return result;
}

