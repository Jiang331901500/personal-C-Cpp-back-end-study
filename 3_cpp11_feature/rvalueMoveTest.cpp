#include <iostream>
#include <cstring>
using namespace std;

class A
{
public:
    A()
    {
        cout << "# a new A by native\n";
        n = new int[5];
    }
    ~A()
    {
        cout << "delete A\n";
        delete [] n;
    }

    /* 复制构造函数，传入左值时 */
    A(const A & a)
    {
        cout << "# a new A by copy\n";
        n = new int[5];
        memcpy(n, a.n, 5);  // 深拷贝
    }

    /* 移动构造函数，传入右值时 */
    A(A && a)
    {
        cout << "# a new A by move\n";
        n = a.n;           // 浅拷贝
        a.n = nullptr;
    }
private:
    int *n;
};

// 这样子写是为了避免返回值优化
A getA(bool flag)
{
    cout << "##### into getA now\n";
    A a;
    A b;

    cout << "##### return now\n";
    if(flag)
        return a;
    else
        return b;
}

template<typename T>
void print(T && n)
{
    cout << "R value " << n << endl;
}
template<typename T>
void print(T & n)
{
    cout << "L value " << n << endl;
}
template<typename T>
void func(T && n)
{
    print(n);
    print(forward<T>(n)); 
}

int main()
{
    #if 0
    {
        cout << "----- copy test\n";
        A a1;
        A a2(a1);      // need copy
    }
    cout << "----- copy over\n";
    {
        cout << "----- move test\n";
        A a3 = getA(true);  // no copy, just move
    }
    cout << "----- move over\n";
    #endif

    #if 0
    A a1;
    A a2 = move(a1);
    #endif

    int n = 10;
    cout << "----------- func(1)\n";
    func(1);
    cout << "----------- func(n)\n";
    func(n);
}