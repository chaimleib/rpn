#include "node.h"
#include <stdlib.h>

node *makeNode(eType type) {
  node *n = (*node) calloc(1, sizeof(node));
  n->type = type;
  return n;
}

void printTree(node *n) {
  int i;
  if (n->type == nodeId) {
    //TODO
  }
}

void deleteTree(node *n) {
  int i;
  if (n->type == nodeId) {
    oprNode opr = n->opr;
    for (i = 0; i < n->opr.nargs; i++) {
      deleteTree(opr->args[i]);
    }
  }
  free(n);
}
