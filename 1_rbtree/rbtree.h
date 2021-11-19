
typedef enum 
{
    RBT_RED = 0,
    RBT_BLACK = 1,
} RBT_COLOR;

typedef int KEY_TYPE;

typedef struct _RBT_NODE RBT_NODE;
struct _RBT_NODE
{
    RBT_NODE* parent;
    RBT_NODE* left;
    RBT_NODE* right;
    RBT_COLOR color;    // RBT_COLOR
    KEY_TYPE key;
};

typedef struct 
{
    RBT_NODE* root;
    RBT_NODE* nil;

    void (*swapData)(RBT_NODE* n1, RBT_NODE* n2);
    int (*keyCompare)(RBT_NODE* target, RBT_NODE* compareWith);
} RBT_TREE;


int RBT_initTree(RBT_TREE* t);
void RBT_traversal(RBT_TREE* t);
RBT_NODE* RBT_insertNode(RBT_TREE* t, RBT_NODE* target);
RBT_NODE* RBT_search(RBT_TREE* t, KEY_TYPE key);
RBT_NODE* RBT_deleteNodeByKey(RBT_TREE* t, KEY_TYPE key);
RBT_NODE* RBT_deleteNode(RBT_TREE* t, RBT_NODE* target);