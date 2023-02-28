#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpn.tab.h"
#include "ast.h"
#include "astring.h"

node* mknod(int type, double d, node *a, node *b, node *c) {
  node *rv = malloc(sizeof(node));
  rv->type = type;
  rv->d = d;
  rv->arg[0] = a;
  rv->arg[1] = b;
  rv->arg[2] = c;
  return rv;
}

typedef struct {
  size_t cap;
  size_t len;
  node **store;
} stack;

stack* mkstack() {
  stack *self = calloc(sizeof(stack), 1);
  if (self == NULL)
    return nomem("mkstack(): ");
  self->cap = 16;
  self->len = 0;
  self->store = calloc(sizeof(node *), self->cap);
  if (self->store == NULL)
    return nomem("mkstack(): ");
  return self;
}

void free_stack(stack *self) {
  free(self->store);
  free(self);
}

// returns 0 on success, non-zero on error
int push(stack *self, node *n) {
  if (!(self->cap = checkRealloc(
      (void **)&self->store, self->cap, sizeof(node *), ++self->len)))
    return !nomem("push(): ");
  self->store[self->len-1] = n;
  return 0;
}

node* pop(stack *self) {
  if (self->len == 0) return NULL;
  return self->store[--self->len];
}

char *ast2str(node *root) {
  // dynamic char arrays
  char *result, *tmp, *indent;
  size_t resultCap = 64, tmpCap = 64, indentCap = 64;
  size_t resultLen = 0, tmpLen = 0, indentLen = 0; // 1 for \0

  // nesting state maps indent level to number of children still expected.
  // this tells us how to modify *indent.
  // Invariants: after any update:
  //   * last item index is nestingLen-1
  //   * parent nodes must append a positive int to nesting
  //   * children nodes must decrement the last item in nesting, if present
  //   * whenever the last item is decremented to 0, --nestingLen
  int *nesting;
  size_t nestingCap = 64, nestingLen = 0;

  // loop vars
  node *n;
  stack *s;

  // temp length var
  size_t l;

  result = calloc(sizeof(char), resultCap);
  tmp = calloc(sizeof(char), tmpCap);
  indent = calloc(sizeof(char), indentCap);
  nesting = calloc(sizeof(int), nestingCap);
  if (result == NULL || tmp == NULL || indent == NULL || nesting == NULL)
    return nomem("ast2str(): init: ");

  s = mkstack();
  if (push(s, root)) return nomem("ast2str(): stackinit: ");

  while ((n = pop(s))) {
    switch (n->type) {

    // leaf nodes
    case NUM:
      // build string to append
      if (nestingLen) {
        // tap the line
        indentLen-=2; // overwrite last 2 chars
        if (!astrcat(&indent, &indentCap, &indentLen,
                     nesting[nestingLen-1]>1 ? "+-" : "\\-", strlen("+-")))
          return nomem("ast2str(): NUM.pre.indent: ");
      }
      // ensure tmp always has enough storage
      l = snprintf(NULL, 0, "%sNUM %0.2f\n", indent, n->d);
      if (!(tmpCap = checkRealloc(
          (void **)&tmp, tmpCap, sizeof(char), l+1)))
        return nomem("ast2str(): NUM.tmp.cr: ");
      tmpLen = snprintf(tmp, l+1, "%sNUM %0.2f\n", indent, n->d);
      if (!astrcat(&result, &resultCap, &resultLen, tmp, tmpLen))
        return nomem("ast2str(): NUM.result: ");
      // update nesting
      if (nestingLen) {
        l = --nesting[nestingLen-1];
        if (l==1) { // change + to \ for branch char
          indent[indentLen-2] = '\\';
        } else if (l==0) { // shorten the indent
          --nestingLen;
          indent[(indentLen-=2)] = '\0';
        }
      }
      break;

    // nodes with children
    case '+':
    case '-':
    case '^':
    case 'n':
      // build next line in tmp
      l = indentLen + strlen("op: 'x'\n");
      if (!(tmpCap = checkRealloc((void **)&tmp, tmpCap, sizeof(char), l+1)))
        return nomem("ast2str(): op.tmp: ");
      tmpLen = snprintf(tmp, l+1, "%sop: '%c'\n", indent, n->type);
      if (!astrcat(&result, &resultCap, &resultLen, tmp, tmpLen))
        return nomem("ast2str(): op.result: ");
      // decrement expected siblings, if applicable
      if (nestingLen) {
        l = --nesting[nestingLen-1];
        if (l==0) { // blank the child branch, while maintaining indent
          memcpy(&indent[indentLen-2], "  ", sizeof("  "));
        } else { // l>0: parent continues w/o connection; indent will increase
          memcpy(&indent[indentLen-2], "| ", sizeof("| "));
        }
      }
      // children to follow!
      if (!(nestingCap = checkRealloc(
          (void **)&nesting, nestingCap, sizeof(int), ++nestingLen)))
        return nomem("ast2str(): op.nesting: ");
      for (int i=2; i>=0; i--) { // count args and increment expected children
        if (!n->arg[i]) continue;
        if (push(s, n->arg[i]))
          return nomem("ast2str(): op.push.arg: ");
        nesting[nestingLen-1]++;
      }
      // update indent
      if (!astrcat(&indent, &indentCap, &indentLen,
                   nesting[nestingLen-1]>1 ? "+-" : "\\-", strlen("+-")))
        return nomem("ast2str(): op.indent: ");
      break;
    default:
      fprintf(stderr, "unrecognized node type\n");
      break;
    }
  }
  free(nesting);
  free(indent);
  free(tmp);
  // caller must free(result)
  free_stack(s);
  return result;
}
