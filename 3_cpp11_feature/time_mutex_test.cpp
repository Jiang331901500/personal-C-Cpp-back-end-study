#include <iostream>
#include <mutex>
#include <thread>

using namespace std;
using ms = chrono::milliseconds;
using this_thread::sleep_for;
using this_thread::get_id;
timed_mutex mtx;

void work()
{
    ms timeout(100);

    while(true)
    {
        if(mtx.try_lock_for(timeout))
        {
            cout << "Thread " << get_id() << " work with lock...\n";
            ms duration(300);
            sleep_for(duration);
            mtx.unlock();
            sleep_for(duration);
        }
        else
        {
            cout << "Thread " << get_id() << " work without lock...\n";
            ms duration(500);
            sleep_for(duration);
        }
    }
}

int main()
{
    thread t1(work);
    thread t2(work);

    t1.join();
    t2.join();
}