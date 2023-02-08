#ifndef AST_H
#define AST_H
typedef struct _node {
  int type;
  double d;
  struct _node *arg[3];
} node;

node* mknod(int type, double d, node *a, node *b, node *c);
char *ast2str(node *);
#endif
