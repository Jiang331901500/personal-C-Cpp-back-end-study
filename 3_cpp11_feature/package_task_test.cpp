#include <iostream>
#include <thread>
#include <future>
using namespace std;
using sec = chrono::seconds;
using this_thread::sleep_for;

int add(int& a, int& b)
{
    cout << "do add...\n";
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
    packaged_task<int(int&, int&)> task(add); // 此时任务函数尚未开始执行
    
    int a = 2;
    int b = 3;
    task(a, b); // task必须先执行，否则future无法就绪，调用get()将永久阻塞
    cout << "task back\n";
    do_something();
    
    future<int> taskResult = task.get_future();
    cout << "task result = " << taskResult.get() << endl;
}