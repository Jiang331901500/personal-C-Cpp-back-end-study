#include <stdio.h>
#include <string.h>
#include "btree.h"

#define M       6       // M阶B树每个节点有M个子节点

int main()
{
    BTREE tree = {0};
    int i = 0;
	char *keys = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    btree_init_tree(&tree, M);
    for(i = 0; i < strlen(keys); i++)
    {
        btree_insert(&tree, keys[i]);
    }

    btree_traversal(&tree);

    for(i = 0; i < strlen(keys); i++)
    {
        printf("--------------------------------------------------------------------- delete %c\n", keys[i]);
        btree_delete(&tree, keys[i]);
        btree_traversal(&tree);
    }

}