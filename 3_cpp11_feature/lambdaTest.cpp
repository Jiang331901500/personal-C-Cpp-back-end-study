#include <iostream>

using namespace std;

int main()
{
    int c = 10;
    int d = 5;

    #if 0
    auto foo = [&](){ c = d; d = 10;};  // 隐式地声明所有变量都按引用方式传递
    foo();
    cout << "c=" << c << "d=" << d << endl;
    #endif

    #if 0
    auto foo = [=](){ return c + d;};  // 隐式地声明所有变量都按值传递
    cout << "foo=" << foo() << endl;
    #endif

    auto foo = [&, d](){ c = d;};  // d 按值传递，其他的都按引用传递
    foo();
    cout << "c=" << c << "d=" << d << endl;

    return 0;
}