# include <iostream>

using namespace std;

enum MODE {SIMPLE_LIST, LOOP_LIST, CROSS_LIST};
struct LinkNode {
    int val;
    LinkNode *next;
};

class myLink
{
private:
    LinkNode* head;
    LinkNode* head2;
    int nodeNum;
    MODE mode;

    LinkNode* buildSimpleList();
    LinkNode* buildLoopList();
    LinkNode* buildCrossList();

public:
    myLink(const MODE & m);
    ~myLink();

    LinkNode* getHead() { return head; }
    LinkNode* getHead2() { return head2; }
    void setHead(LinkNode* h) { head = h; }
    void setHead2(LinkNode* h) { head2 = h; }

    void clearList();
    void showList() const;
    int countListNodeNum(LinkNode* h) const;
    LinkNode* getNthNode(LinkNode* h, const int & n) const;

    bool isListLooped(LinkNode* h) const;                          // 链表是否有环
    LinkNode* isListCrossed(LinkNode* h1, LinkNode* h2) const;     // 链表是否有重叠
    LinkNode* rfindNodeNo(LinkNode* h, const int & k) const;       // 找到倒数第k个节点
    LinkNode* reverseList(LinkNode* h);                            // 反转单向链表
};

myLink::myLink(const MODE & m = SIMPLE_LIST) : nodeNum(10), mode(m)
{
    head2 = nullptr;
    head = new LinkNode;
    head->val = 0;
    head->next = nullptr;
    
    switch(m)
    {      
        case LOOP_LIST:
            buildLoopList();
            break;

        case CROSS_LIST:
            buildCrossList();
            break;

        default:
            buildSimpleList();
            break;
    }
}

myLink::~myLink()
{
    clearList();
}

void myLink::showList() const
{
    cout << "LinkList:: ";
    int i = 0;
    LinkNode* p = head;

    while(p != nullptr)
    {
        cout << i << "_" << p->val << " -> ";
        p = p->next;
        i++;
    }

    cout << endl;
}

LinkNode * myLink::buildSimpleList()
{
    LinkNode *p = head;
    for(int i = 1; i < nodeNum; i++)
    {
        LinkNode *q = new LinkNode;
        q->val = i * 2;
        q->next = nullptr;

        p->next = q;
        p = q;
    }

    return head;
}

LinkNode * myLink::buildLoopList()
{
    LinkNode *p = head;
    for(int i = 1; i < nodeNum; i++)
    {
        LinkNode *q = new LinkNode;
        q->val = i * 2;
        q->next = nullptr;

        p->next = q;
        p = q;
    }

    p->next = head->next->next->next;   // just for test

    return head;
}

LinkNode * myLink::buildCrossList()
{
    buildSimpleList();

    head2 = new LinkNode;
    head2->val = 0;
    head2->next = nullptr;

    LinkNode *p = head2;
    for(int i = 1; i < 8; i++)
    {
        LinkNode *q = new LinkNode;
        q->val = i * 2;
        q->next = nullptr;

        p->next = q;
        p = q;
    }

    p->next = head->next->next->next->next;

    return head;
}

void myLink::clearList()
{
    LinkNode *p = head;
    while(p != nullptr)
    {
        LinkNode *q = p;
        p = p->next;
        q->next = nullptr;
        delete q;
    }
    head = nullptr;

    p = head2;
    while(p != nullptr)
    {
        LinkNode *q = p;
        p = p->next;
        delete q;
    }
    head2 = nullptr;
}

/* 快慢指针判断链表是否有环 */
bool myLink::isListLooped(LinkNode* h) const
{
    if(h == nullptr)
    {
        return false;
    }

    LinkNode *slow = h;         // 慢指针，一次前进1个节点
    LinkNode *fast = h->next;   // 快指针，一次前进2个节点

    while(slow != nullptr && fast != nullptr)
    {
        if(slow == fast)
        {
            return true;
        }
        slow = slow->next;
        fast = (fast->next != nullptr) ? fast->next->next : fast->next;
    }

    return false;
}

/* O(n)复杂度判断链表是否重叠 */
LinkNode* myLink::isListCrossed(LinkNode* h1, LinkNode* h2) const
{
    int nodeNum1 = countListNodeNum(h1);    // 计算节点数目
    int nodeNum2 = countListNodeNum(h2);    // 计算节点数目
    int nodeNumMinus = 0;

    if(nodeNum1 > nodeNum2)
    {
        nodeNumMinus = nodeNum1 - nodeNum2;
        h1 = getNthNode(h1, nodeNumMinus);  // 链表1先走几个节点
    }
    else
    {
        nodeNumMinus = nodeNum2 - nodeNum1;
        h2 = getNthNode(h2, nodeNumMinus);  // 链表2先走几个节点
    }

    while(h1 != nullptr && h2 != nullptr)
    {
        if(h1 == h2)
        {
            return h1;
        }

        h1 = h1->next;
        h2 = h2->next;
    }

    return nullptr;
}

// n from 0
LinkNode* myLink::getNthNode(LinkNode* h, const int & n) const
{
    int i = 0;

    while(h != nullptr && i < n)
    {
        h = h->next;
        i++;
    }

    return h;
}

int myLink::countListNodeNum(LinkNode* h) const
{
    int cnt = 0;

    while(h != nullptr)
    {
        cnt++;
        h = h->next;
    }

    return cnt;
}

LinkNode* myLink::rfindNodeNo(LinkNode* h, const int & k) const
{
    LinkNode* tail;

    if(k <= 0)
    {
        return nullptr;
    }

    tail = getNthNode(h, k - 1);    // N 从0开始数，所以 k-1
    if(tail == nullptr)
    {
        return nullptr;
    }

    while(tail->next != nullptr)
    {
        tail = tail->next;
        h = h->next;
    }
    
    return h;
}

LinkNode* myLink::reverseList(LinkNode* h)
{
    LinkNode* prev = nullptr;
    LinkNode* current = h;
    LinkNode* next;

    while (current != nullptr)
    {
        next = current->next;
        current->next = prev;
        prev = current;
        current = next;
    }
    
    return prev;
}

int main()
{
    cout << "====== simple list test ========\n";
    myLink ml(SIMPLE_LIST);
    cout << "SIMPLE_LIST before reverse:\n";
    ml.showList();
    
    ml.setHead( ml.reverseList(ml.getHead()) );
    cout << "SIMPLE_LIST after reverse:\n";
    ml.showList();

    cout << "Is list looped? : " << ml.isListLooped(ml.getHead()) << endl;

    cout << "Find node 4th value in reverse : " << ml.rfindNodeNo(ml.getHead(), 4)->val << endl;

    cout << "\n====== looped list test ========\n";
    // ml = myLink(LOOP_LIST); // 因为还没有重载赋值运算符，不能这么做
    myLink ml2(LOOP_LIST);
    cout << "Is list looped? : " << ml2.isListLooped(ml2.getHead()) << endl;

    cout << "\n====== cross list test ========\n";
    myLink ml3(CROSS_LIST);
    cout << "Is list corssed at : " << ml3.isListCrossed(ml3.getHead(), ml3.getHead2())->val << endl;

    return 0;
}