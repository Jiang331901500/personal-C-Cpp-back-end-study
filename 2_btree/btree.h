
typedef int KEY_TYPE;

typedef struct btree_node BTREE_NODE;
struct btree_node
{
    KEY_TYPE* keys;
    BTREE_NODE** children;

    int keyNum;        // 当前节点key的数量
    int isLeaf;
} ;


typedef struct btree
{
    BTREE_NODE* root;
    int kMaxNum;        // 节点的最大key数量
    int kMinNum;        // 除根节点外，每个节点的最小key数量
    int kMidIdx;        // 节点中间可以用于分裂的那个key索引
    int order;          // 阶数
} BTREE;

void btree_insert(BTREE* t, KEY_TYPE key);
int btree_init_tree(BTREE* t, int order);
int btree_delete(BTREE* t, KEY_TYPE key);
void btree_traversal(BTREE* t);
