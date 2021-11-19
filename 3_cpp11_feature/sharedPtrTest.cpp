#include <iostream>
#include <memory>

using namespace std;

class A : public enable_shared_from_this<A>
{
public:
    ~A() { cout << "# A destroyed.\n"; }
    shared_ptr<A> getSelfPtr()
    {
        return shared_from_this();
    }
};


int foo()
{
    /* ... some exceptions may happen*/
    return 0;
}
void function(const shared_ptr<int>& p, int n)
{
    cout << "*p = " << *p << endl;
}

int main()
{
    #if 0
    shared_ptr<int> p1(new int(100));
    shared_ptr<int> p2 = p1;
    shared_ptr<int> p3;
    p3.reset(new int(10));
    auto sp = make_shared<int>(20);

    if(p3) 
        cout << "p3 is not null\n";
    else
        cout << "p3 is null\n";
    p3.reset(); // deinit
    if(p3) 
        cout << "p3 is not null\n";
    else
        cout << "p3 is null\n";
    #endif

    #if 0
    int* op = new int(5);
    shared_ptr<int> pw1(op);
    shared_ptr<int> pw2(op);
    cout << "pw1 ref count: " << pw1.use_count() << endl;
    cout << "pw2 ref count: " << pw2.use_count() << endl;
    cout << "*pw2 = " << *pw2 << endl;
    pw1.reset();
    cout << "pw1 ref count: " << pw1.use_count() << endl;
    cout << "pw2 ref count: " << pw2.use_count() << endl;
    cout << "*pw2 = " << *pw2 << endl;

    function(shared_ptr<int>(new int(20)), foo());
    #endif

    shared_ptr<A> pa(new A);
    shared_ptr<A> pb = pa->getSelfPtr();

}

