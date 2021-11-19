#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rbtree.h"

static int _leftRotate(RBT_TREE* t, RBT_NODE* x)
{
    if(t->nil == x->right)
        return -1;

    RBT_NODE* p = x->parent;
    RBT_NODE* y = x->right;
    RBT_NODE* z = y->left;

    if(p == t->nil)
        t->root = y;
    else if(x == p->left)
        p->left = y;
    else
        p->right = y;
    y->parent = p;
    y->left = x;
    x->parent = y;
    x->right = z;
    if(z != t->nil)
        z->parent = x;

    return 0;
}

static int _rightRotate(RBT_TREE* t, RBT_NODE* y)
{
    if(t->nil == y->left)
        return -1;

    RBT_NODE* p = y->parent;
    RBT_NODE* x = y->left;
    RBT_NODE* z = x->right;

    if(p == t->nil)
        t->root = x;
    if(y == p->right)
        p->right = x;
    else
        p->left = x;
    x->parent = p;
    x->right = y;
    y->parent = x;
    y->left = z;
    if(z != t->nil)
        z->parent = y;

    return 0;
}

/* 插入后对节点进行调整 */
static int _adjustAfterInsert(RBT_TREE* t, RBT_NODE* node)
{
    while(node->parent->color == RBT_RED)
    {
        if(node->parent == node->parent->parent->left)  // 父节点是祖父节点的左子树
        {
            RBT_NODE* uncle = node->parent->parent->right;
            if(uncle->color == RBT_RED) // 叔节点是红色，情况2
            {
                uncle->color = node->parent->color = RBT_BLACK;
                node->parent->parent->color = RBT_RED;
                node = node->parent->parent;
            }
            else                        // 叔节点是黑色
            {
                if(node == node->parent->right) // 情况4
                {
                    node = node->parent;
                    _leftRotate(t, node);
                }
                // 情况3
                node->parent->color = RBT_BLACK;
                node->parent->parent->color = RBT_RED;
                _rightRotate(t, node->parent->parent);
            }
        }
        else
        {
            RBT_NODE* uncle = node->parent->parent->left;
            if(uncle->color == RBT_RED) // 叔节点是红色
            {
                uncle->color = node->parent->color = RBT_BLACK;
                node->parent->parent->color = RBT_RED;
                node = node->parent->parent;
            }
            else                        // 叔节点是黑色
            {
                if(node == node->parent->left)
                {
                    node = node->parent;
                    _rightRotate(t, node);
                }

                node->parent->color = RBT_BLACK;
                node->parent->parent->color = RBT_RED;
                _leftRotate(t, node->parent->parent);
            }
        }
    }

    t->root->color = RBT_BLACK;
}

static void _adjustAfterDelete(RBT_TREE* t, RBT_NODE* succesor)
{
    RBT_NODE* bro;
    RBT_NODE* parent;

    while(succesor->color == RBT_BLACK && t->root != succesor)
    {   
        parent = succesor->parent;
        if(succesor == parent->right)
        {
            bro = parent->left;
            
            if(bro->color == RBT_RED)   // 情况1
            {
                bro->color = RBT_BLACK;
                bro->right->color = RBT_RED;
                _rightRotate(t, parent);
                break;     // OVER
            }
            else
            {
                if(bro->left->color == RBT_BLACK && bro->right->color == RBT_BLACK) // 情况2
                {
                    bro->color = RBT_RED;
                    if(parent->color == RBT_RED)    // 遇到父节点为红
                    {
                        parent->color = RBT_BLACK;
                        break;     // OVER
                    }
                    // need to continue, don't break;
                    succesor = parent;
                }
                else if(bro->left->color == RBT_BLACK)  // 情况3
                {
                    bro->color = parent->color;
                    bro->right->color = RBT_BLACK;
                    _leftRotate(t, bro);
                    _rightRotate(t, parent);
                    break;     // OVER
                }
                else    // 情况4
                {
                    bro->color = parent->color;
                    parent->color = RBT_BLACK;
                    bro->left->color = RBT_BLACK;
                    _rightRotate(t, parent);
                    break;     // OVER
                }
            }
        }
        else
        {
            bro = parent->right;
            
            if(bro->color == RBT_RED)   // 情况1
            {
                bro->color = RBT_BLACK;
                bro->left->color = RBT_RED;
                _leftRotate(t, parent);
                break;     // OVER
            }
            else
            {
                if(bro->right->color == RBT_BLACK && bro->left->color == RBT_BLACK) // 情况2
                {
                    bro->color = RBT_RED;
                    if(parent->color == RBT_RED)    // 遇到父节点为红
                    {
                        parent->color = RBT_BLACK;
                        break;     // OVER
                    }
                    // need to continue, don't break;
                    succesor = parent;
                }
                else if(bro->right->color == RBT_BLACK)  // 情况3
                {
                    bro->color = parent->color;
                    bro->left->color = RBT_BLACK;
                    _rightRotate(t, bro);
                    _leftRotate(t, parent);
                    break;     // OVER
                }
                else    // 情况4
                {
                    bro->color = parent->color;
                    parent->color = RBT_BLACK;
                    bro->right->color = RBT_BLACK;
                    _leftRotate(t, parent);
                    break;     // OVER
                }
            }
        }
    }
}

static RBT_NODE* _getSuccessor(RBT_TREE* t, RBT_NODE* node)
{
    RBT_NODE* succ = node;

    if(node->right != t->nil)
    {
        succ = node->right;
        while(succ->left != t->nil)
            succ = succ->left;
    }

    return succ;
}

/* 定义swapNode进行节点数据的交换是因为：
    我们定义的RBT结构体是一个通用RBT，内部不含业务数据，交换key时无法交换数据，因此直接交换节点
*/
static void _swapNode(RBT_TREE* t, RBT_NODE* node1, RBT_NODE* node2)
{
    KEY_TYPE keyTemp;

    keyTemp = node1->key;
    node1->key = node2->key;
    node2->key = keyTemp;

    t->swapData(node1, node2);
}

static void _traversal(RBT_TREE* t, RBT_NODE* node, int lvl)
{
    int i;
    if(node == t->nil)
    {
        for(i = 0; i < lvl; i++)
            printf("\t");
        printf("nil\n");
        return ;
    }
    _traversal(t, node->left, lvl + 1);
    for(i = 0; i < lvl; i++)
        printf("\t");
    printf("%02d:%c\n", node->key, (node->color == RBT_BLACK)?'B':'R');
    _traversal(t, node->right, lvl + 1);
}

RBT_NODE* RBT_insertNode(RBT_TREE* t, RBT_NODE* target)
{
    RBT_NODE* node = t->root;
    RBT_NODE* prev = node;

    // 遍历找到插入点
    while (node != t->nil)
    {
        prev = node;
        if(target->key < node->key)
            node = node->left;
        else if(target->key > node->key)
            node = node ->right;
        else
        {
            // 已存在的key暂时按丢弃处理
            printf("# ERROR: repeat key, abanded.\n");
            return node;
        }
    }

    // 插入新节点
    target->parent = prev;
    if(prev == t->nil)
        t->root = target;  // 传入的是空树，返回根节点
    if(target->key < prev->key)
        prev->left = target;
    else
        prev->right = target;

    // 插入节点参数初始化
    target->left = t->nil;
    target->right = t->nil;
    target->color = RBT_RED;  // 默认插入红色
    
    // adjust - 红黑树性质恢复
    _adjustAfterInsert(t, target);

    return target;
}

RBT_NODE* RBT_search(RBT_TREE* t, KEY_TYPE key)
{
    RBT_NODE* node;

    node = t->root;

    while(node != t->nil)
    {
        if(key < node->key)
            node = node->left;
        else if(key > node->key)
            node = node->right;
        else
            return node;
    }

    return t->nil;    //  not found
}

RBT_NODE* RBT_deleteNode(RBT_TREE* t, RBT_NODE* target)
{
    RBT_NODE* succesor = target;
    RBT_NODE* parent;

    if(target == t->nil)
        return t->nil;

    /* 左右子树不为nil，则找到后继节点并与当前节点替换 */
    if(succesor->left != t->nil || succesor->right != t->nil)
    {
        succesor = _getSuccessor(t, succesor);  // 寻找后继节点
        _swapNode(t, target, succesor);     // 交换两个节点的内容
        target = succesor;
    }

    if(target->left != t->nil)
        succesor = target->left;    // 待删节点为情况(1)
    else if(target->right != t->nil)
        succesor = target->right;   // 待删节点为情况(2)
    else
        succesor = t->nil;          // 待删节点为情况(3)

    parent = target->parent;
    succesor->parent = parent;
    if(parent == t->nil)
        t->root = succesor;         // 删除根节点的情况
    else if(target == parent->left)
        parent->left = succesor; 
    else
        parent->right = succesor;
    // 至此target已从树中脱离，其位置被succesor替代

    succesor->color = RBT_BLACK;    // 对于情况(1)和(2)，删除后，替补的后继节点一定要涂成黑色

    /* 只有待删节点为情况(3)且删除的节点颜色为黑时需要调整 */
    if(succesor == t->nil && target->color == RBT_BLACK)
        _adjustAfterDelete(t, succesor);
    
    return target;
}

RBT_NODE* RBT_deleteNodeByKey(RBT_TREE* t, KEY_TYPE key)
{
    RBT_NODE* target = RBT_search(t, key);

    return RBT_deleteNode(t, target);
}


int RBT_initTree(RBT_TREE* t)
{
    int i;
    if(NULL == t)
        return -1;

    // 初始化nil节点
    t->nil = (RBT_NODE*)malloc(sizeof(RBT_NODE));
    if(t->nil == NULL)
        return -1;
    t->nil->color = RBT_BLACK;
    t->root = t->nil;
    t->root->left = t->root->right = t->root->parent = t->nil;

    return 0;
}

void RBT_traversal(RBT_TREE* t)
{
    if(t == NULL)
        return;

    _traversal(t, t->root, 0);
}
