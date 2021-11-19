#include <iostream>
#include <thread>
#include <future>
using namespace std;
using sec = chrono::seconds;
using this_thread::sleep_for;

int work_add()
{
    sleep_for(sec(2));
    return 1 + 1;
}

int work_add2(int& a, int& b)
{
    sleep_for(sec(2));
    return a + b;
}

void do_something()
{
    sleep_for(sec(2));
    cout << "something else going on...\n";
}

int main()
{
    future<decltype(work_add())> result = async(launch::async, work_add);
    do_something();
    cout << "work_add result = " << result.get() << endl;

    int a = 2;
    int b = 3;
    future<decltype(work_add2(a ,b))> result2 = async(launch::deferred, work_add2, ref(a), ref(b));
    do_something();
    result2.wait();
    cout << "result2 wait over\n";  // 
    cout << "work_add2 result = " << result2.get() << endl;
}