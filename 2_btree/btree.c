#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "btree.h"

static BTREE_NODE* _create_node(BTREE* t, int isLeaf)
{
    BTREE_NODE* node = (BTREE_NODE*)malloc(sizeof(BTREE_NODE));
    if(node == NULL)
        assert(0);

    node->keys = (KEY_TYPE*)calloc(t->kMaxNum, sizeof(KEY_TYPE));
    node->children = (BTREE_NODE**)calloc(t->order, sizeof(BTREE_NODE*));
    if(node->keys == NULL || node->children == NULL)
        assert(0);

    memset(node->keys, 0, t->kMaxNum * sizeof(KEY_TYPE));
    memset(node->children, 0, t->order * sizeof(BTREE_NODE*));
    node->keyNum = 0;
    node->isLeaf = isLeaf;

    return node;
}

// parent 节点的下标为n的孩子进行分裂
static void _split_child(BTREE* t, BTREE_NODE* parent, int n)
{
    int i, j;
    if(n < 0 || n > t->kMaxNum) assert(0);

    BTREE_NODE* splitChild = parent->children[n];
    BTREE_NODE* newSibling = _create_node(t, splitChild->isLeaf);
    i = parent->keyNum - 1;
    
    // 先将父节点中处于分裂节点之后的key都往后移以腾出空间
    while(i >= n)
    {
        parent->keys[i + 1] = parent->keys[i];
        parent->children[i + 2] = parent->children[i + 1];
        i--;
    }   // 此时插入key的位置为i+1, 插入的key左右两边的子节点为i+1和i+2，i+1节点不变，i+2指向新节点
    parent->keys[i + 1] = splitChild->keys[t->kMidIdx]; // 分裂节点的中间位置的key上移到父节点
    parent->children[i + 2] = newSibling;

    // 将分裂节点后半部分拷贝到新节点中
    for(i = t->kMidIdx + 1, j = 0; i < t->kMaxNum; i++, j++)
    {
        newSibling->keys[j] = splitChild->keys[i];
        newSibling->children[j] = splitChild->children[i];
    }
    newSibling->children[j] = splitChild->children[i]; // 最后一个关键字右侧的子节点别忘了拷贝

    splitChild->keyNum = t->kMinNum;
    newSibling->keyNum = t->kMinNum;
    parent->keyNum++;
}

// 若当前节点未满，则使用_insert_notfull进行插入操作，否则先分裂后再向其子节点进行插入操作
static void _insert_notfull(BTREE* t, BTREE_NODE* node, KEY_TYPE key)
{
    int i;

    // 一直往下递归找到叶子节点
    if(node->isLeaf)
    {
        i = node->keyNum - 1;
        while( i >= 0 && key < node->keys[i])
        {
            node->keys[i + 1] = node->keys[i];  // 找插入位置的过程中先把较大的关键字后移以腾出空间
            i--;
        }

        node->keys[i + 1] = key;
        node->keyNum++; // 关键字数量加1
    }
    else
    {
        // 找到对应的子树
        i = node->keyNum - 1;
        while( i >= 0 && key < node->keys[i]) i--;
        if(node->children[i + 1]->keyNum == t->kMaxNum) // 判断子树是不是满的，如果是满的，要先将子树分裂
        {
            _split_child(t, node, i + 1);
            i++; // 因为子节点分裂后有一个key上升到待插入节点的后面，因此索引要加1
        }
        
        // 递归进入子树
        if(key > node->keys[i]) // 大于当前key，插到其右边(i + 2)
            _insert_notfull(t, node->children[i + 1], key);
        else                            // 大于当前key，插到其右边(i + 1)
            _insert_notfull(t, node->children[i], key);
    }
}

static void _traversal(BTREE_NODE* node, int lvl)
{
    int i;
    if(node == NULL)
    {
        return ;
    }

    for(i = 0; i < lvl; i++)
        printf("\t\t\t\t");

    printf("layer=%02d keynum=%02d is_leaf=%c\n", lvl + 1, node->keyNum, node->isLeaf?'Y':'N');
    for(i = 0; i < lvl; i++)
        printf("\t\t\t\t");
    
    for(i = 0; i < node->keyNum; i++)
        printf("%c ", node->keys[i]);
    printf("\n");

    for(i = 0; i <= node->keyNum; i++)
    {
        _traversal(node->children[i], lvl + 1);
    }

}

static void _destroy_node(BTREE_NODE* parent)
{
    if(parent)
    {
        if(parent->keys)
            free(parent->keys);
        if(parent->children)
            free(parent->children);

        free(parent);
    }
}

// 将parent节点的索引为n的元素下放并与其左右子节点合并
static BTREE_NODE* _merge_children(BTREE* t, BTREE_NODE* parent, int n)
{
    int i, j;
    BTREE_NODE* leftChild = parent->children[n];
    BTREE_NODE* rightChild = parent->children[n + 1];

    leftChild->keys[leftChild->keyNum] = parent->keys[n];
    for(i = leftChild->keyNum + 1, j =0; j < rightChild->keyNum; i++, j++)
    {
        leftChild->keys[i] = rightChild->keys[j];
        leftChild->children[i] = rightChild->children[j];   // 右节点的子节点转移到左节点后面
    }
    leftChild->children[i] = rightChild->children[j];   // 右边最后一个子节点
    leftChild->keyNum += 1 + rightChild->keyNum;        // 节点数增加，1为父节点合入

    _destroy_node(rightChild);  // 将右子节点释放掉

    for(i = n; i < parent->keyNum - 1; i++)
    {
        parent->keys[i] = parent->keys[i + 1];
        parent->children[i + 1] = parent->children[i + 2];
    }
    parent->children[i + 1] = NULL;
    parent->keyNum--;

    // !! 如果是根节点的最后一个元素被合并入子节点，根节点将为空
    // 此时需要释放原来的根节点，将新的子节点作为根节点
    if(parent->keyNum == 0)
    {
        t->root = leftChild;
        _destroy_node(parent);
    }

    return NULL;
}

static void _delete_key(BTREE* t, BTREE_NODE* node, KEY_TYPE key)
{
    int i;
    BTREE_NODE* child;
    BTREE_NODE* leftSibling;
    BTREE_NODE* rightSibling;
    int targetIdx = 0;  // targetIdx要么是当前节点中待删key的索引

    if (node == NULL || node->keyNum == 0)
        return ;

    // 从节点的元素中从前往后找到第一个大于key的元素
    while(targetIdx < node->keyNum && node->keys[targetIdx] < key) 
        targetIdx++;

    // 1 - 找了到匹配的key
    if(targetIdx < node->keyNum && key == node->keys[targetIdx])
    {
        // 1.1 - 看当前节点是否是叶子节点
        if(node->isLeaf)    // 是叶子节点，执行删除操作
        {
            for(i = targetIdx; i < node->keyNum - 1; i++)
                node->keys[i] = node->keys[i + 1];
            node->keyNum--;
            // 此处理论上不需要判断节点中的key是否都被删完了，
            // 因为这种情况只有root节点会出现，而我们可以选择不将root节点释放
            // 为什么删除叶节点的元素时不需要判断key的数量是否少于下限kmin？
            // 因为下面的分支2在向下查找的过程中会保证子节点的元素数量一定大于kmin
        }
        else    // 1.2 - 不是叶子节点，而是中间节点，则寻找后继节点的key来替换
        {
            // 在该key左右两边的子节点中选一个后继节点
            if(node->children[targetIdx + 1]->keyNum >= t->kMinNum)
            {   // 1.2.1 - 优先选右边，即子节点的索引为 targetIdx + 1，但要求子节点的元素数量大于下限kmin
                node->keys[targetIdx] = node->children[targetIdx + 1]->keys[0]; // 后继节点的元素替换
                _delete_key(t, node->children[targetIdx + 1], node->children[targetIdx + 1]->keys[0]); // 转而去后继节点种删除
            }
            else if(node->children[targetIdx]->keyNum >= t->kMinNum)
            {   // 1.2.2 - 右边不行就选左边，即子节点的索引为 targetIdx，同样要求子节点的元素数量大于下限kmin
                node->keys[targetIdx] = node->children[targetIdx]->keys[0]; // 后继节点的元素替换
                _delete_key(t, node->children[targetIdx], node->children[targetIdx]->keys[node->keyNum - 1]); // 转而去后继节点中删除
            }
            else
            {   // 1.2.3 - 左右子节点都不行，那就与他们合并吧
                _merge_children(t, node, targetIdx);
                _delete_key(t, node->children[targetIdx], key); // 继续到合并的节点中去删除
            }
        }
    }
    else    // 2 - 当前节点中没有待删的key，需要到子节点中去找
    {
        // 显然要查找的下一个子节点索引为targetIdx
        child = node->children[targetIdx];
        if(child == NULL)
        {   // 都找到叶子节点的了还没找到，child就是NULL了
            printf("there is no such key in tree!\n");
            return ;
        }

        // 进入字节点查找前，先确保子节点的key数量大于kmin
        if(child->keyNum <= t->kMinNum)  // 2.1 - 子节点不满足要求，需要扩充子节点
        {   // 只能去左右兄弟节点那里拉壮丁
            leftSibling = NULL;     // 左右兄弟节点的指针初始化为NULL，以便判断是否存在
            rightSibling = NULL;
            if (targetIdx - 1 >= 0)
				leftSibling = node->children[targetIdx - 1];
			if (targetIdx + 1 <= node->keyNum) 
				rightSibling = node->children[targetIdx + 1];

            if(rightSibling && rightSibling->keyNum > t->kMinNum)
            {   // 2.1.1 - 向右兄弟借位，对应的子节点索引是targetIdx+1。前提是右兄弟要存在哈
                child->keys[child->keyNum] = node->keys[targetIdx];
                child->children[child->keyNum + 1] = rightSibling->children[0];
                node->keys[targetIdx] = rightSibling->keys[0];
                for(i = 0; i < rightSibling->keyNum - 1; i++)
                {
                    rightSibling->keys[i] = rightSibling->keys[i + 1];
                    rightSibling->children[i] = rightSibling->children[i + 1];
                }
                rightSibling->children[i] = rightSibling->children[i + 1];  // 最后右边的一个子节点别忘了
                rightSibling->children[i + 1] = NULL;

                child->keyNum++;
                rightSibling->keyNum--;
            }
            else if(leftSibling && leftSibling->keyNum > t->kMinNum)
            {   // 2.1.2 - 向左兄弟借位，前提也是左兄弟要存在
                for(i = child->keyNum; i > 0; i--)
                {
                    child->keys[i] = child->keys[i - 1];
                    child->children[i + 1] = child->children[i];
                }
                child->children[i + 1] = child->children[i];  // 最后左边的一个子节点别忘了

                child->keys[0] = node->keys[targetIdx - 1];
                child->children[0] = leftSibling->children[leftSibling->keyNum];
                leftSibling->children[leftSibling->keyNum] = NULL;
                node->keys[targetIdx - 1] = leftSibling->keys[leftSibling->keyNum - 1];

                child->keyNum++;
                leftSibling->keyNum--;
            }
            else
            {   // 2.1.3 - 左右兄弟都不行，还是合并他们吧
                if(rightSibling)
                    _merge_children(t, node, targetIdx);    // 将右节点合并进来
                else if(leftSibling)
                {
                    _merge_children(t, node, targetIdx - 1);
                    child = leftSibling;    // 与左节点合并时，是合入左节点，因此子节点指针要转移
                }
                else
                    assert(0); // 这种情况不可能出现
            }
        }

        _delete_key(t, child, key); // 进入到子节点继续查找待删的key
    }
}

int btree_delete(BTREE* t, KEY_TYPE key)
{
    if(t == NULL)
        return -1;
    _delete_key(t, t->root, key);
    return 0;
}

void btree_insert(BTREE* t, KEY_TYPE key)
{
    BTREE_NODE* root = t->root;
    if(root->keyNum == t->kMaxNum)  // 若根节点已满则先做单独处理
    {
        // 创建一个新节点作为root，然后以该节点为父节点进行分裂操作
        root = _create_node(t, 0);
        root->children[0] = t->root;    // 新节点的第一个子节点指向原来的根节点
        t->root = root;

        _split_child(t, root, 0);

        // 根节点分裂完成，继续向下找到可以插入的未满叶节点
        if(key < root->keys[0])
            _insert_notfull(t, root->children[0], key);
        else
            _insert_notfull(t, root->children[1], key);
    }
    else
    {
        _insert_notfull(t, root, key);
    }
}

int btree_init_tree(BTREE* t, int order)
{
    if(order < 2 || t == NULL)
        return -1;

    t->order = (order & 1) + order;   // 确保阶数为偶数
    t->kMidIdx = (t->order >> 1) - 1; // 当一个节点已满需要分裂时，kMidIdx是位于中间那个key的索引
    t->kMinNum = t->kMidIdx;          // 节点元素数量的下限kmin（除根节点外）
    t->kMaxNum = order - 1;           // 最大元素

    t->root = _create_node(t, 1);

    return 0;
}



void btree_traversal(BTREE* t)
{
    _traversal(t->root, 0);
}