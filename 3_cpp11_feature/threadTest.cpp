#include <iostream>
#include <thread>

using namespace std;

void work1()
{
    cout << "This is work1\n";
}

void work2(int a, int b)
{
    cout << "work2 a + b = " << a + b << endl;
}

int main()
{
    thread t1(work1);
    cout << "thread id " << t1.get_id() << " running...\n";
    if(t1.joinable())
        t1.join();

    
    thread t2(work2, 1, 2);
    cout << "thread id " << t2.get_id() << " running...\n";
    if(t2.joinable())
        t2.join();
}

