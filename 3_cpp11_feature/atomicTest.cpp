#include <iostream>
#include <thread>
#include <atomic>
using namespace std;
using sec = chrono::seconds;
using this_thread::sleep_for;

#if 0
atomic<int> foo(0); // 正确的初始化方式

void put_foo(int x)
{
    foo.store(x);
}

void get_foo()
{
    int x;
    do
    {
        x = foo.load();
    } while (x == 0);
    cout << "foo:" << x << endl;
}
#endif

atomic<int> a(0);
atomic<int> b(0);
void f1()
{
    int t = 1;
    sleep_for(sec(2));
    a.store(t, memory_order_relaxed);
    b.store(2, memory_order_relaxed);
}

void f2()
{
	while(b.load(memory_order_relaxed) == 0);
	cout << "a=" << a.load(memory_order_relaxed) << ", b=" << b.load(memory_order_relaxed) << endl;
}

int main()
{
    thread t1(f1);
    thread t2(f2);

    t1.join();
    t2.join();
    return 0;
}