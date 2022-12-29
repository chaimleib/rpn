typedef enum { doubleId, nodeId } eType;

typedef struct {
  int oper;
  int nargs;
  struct _node *args[1];
} oprNode;

typedef struct _node {
  eType type;
  union {
    double d;
    oprNode opr;
  };
} node;

node *makeNode(eType type);
void deleteTree(node *n);
