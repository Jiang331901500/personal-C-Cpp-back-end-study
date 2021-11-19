#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"


typedef struct
{
    RBT_NODE node;

    // node data
    void* value;
} NODE;

typedef struct 
{
    RBT_TREE root;

    // Tree attribute
} TREE;

static int _keyCompare(RBT_NODE* a, RBT_NODE* b)
{
    // need to implement

    return 0;
}

static void _swapData(RBT_NODE* n1, RBT_NODE* n2)
{
    NODE* node1 = (NODE*)n1;
    NODE* node2 = (NODE*)n2;
    // need to implement method for exchanging data in NODE

}

NODE* APP_createNewNode(KEY_TYPE key)
{
    NODE* node = (NODE*)malloc(sizeof(NODE));
    if(NULL == node)
        return NULL;

    node->node.key = key;

    return node;
}

NODE* APP_insertNode(RBT_TREE* t, KEY_TYPE key)
{
    NODE* newNode;
    
    newNode = APP_createNewNode(key);
    if(newNode == NULL)
        return NULL;

    RBT_insertNode(t, &newNode->node);
    return newNode;
}

int APP_initTree(TREE* tree, KEY_TYPE key[], int n)
{
    int i;
    if(NULL == tree)
        return -1;

    RBT_initTree(&tree->root);
    tree->root.swapData = _swapData;

    // 向树中插入传入的数据
    for(i = 0; i < n; i++)
    {
        APP_insertNode(&tree->root, key[i]);
    }
}


void *APP_getData(TREE* tree, KEY_TYPE key)
{
    RBT_NODE* node = RBT_search(&tree->root, key);
    if(node == tree->root.nil)
        return NULL;
    
    return ((NODE*)node)->value;
}

void APP_delete(TREE* tree, KEY_TYPE key)
{
    NODE* node;
    RBT_NODE* rbtNode = RBT_deleteNodeByKey(&tree->root, key);
    if(rbtNode != NULL && rbtNode != tree->root.nil)
    {
        node = (NODE*)rbtNode;
        free(node);
    }
}

int main()
{
    TREE appTree;

    int i;
    KEY_TYPE keys[] = {24,25,13,35,23, 26,67,47,38,98, 20,19,17,49,12, 21,9,18,14,15};
    int size = sizeof(keys)/sizeof(KEY_TYPE);

    APP_initTree(&appTree, keys, size);
    RBT_traversal(&appTree.root);

    for(i = 0; i < size; i++)
    {
        printf("----------------------------------------- delete %d\n", keys[i]);
        APP_delete(&appTree, keys[i]);
        RBT_traversal(&appTree.root);
    }
}