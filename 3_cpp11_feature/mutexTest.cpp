#include <iostream>
#include <mutex>
#include <thread>

using namespace std;

volatile int counter = 0;
mutex mtx;

void increaseCounterByNoLock()
{
    for(int i = 0; i < 10000; i++)
    {
        counter++;
    }
}

void increaseCounterByLock()
{
    for(int i = 0; i < 10000; i++)
    {
        mtx.lock();
        counter++;
        mtx.unlock();
    }
}

void increaseCounterByTryLock()
{
    for(int i = 0; i < 10000; i++)
    {
        if(mtx.try_lock())
        {
            counter++;
            mtx.unlock();
        }
    }
}

int main()
{
    thread threads[10];

    for(auto& t : threads)
        t = thread(increaseCounterByNoLock);
    for(auto& t : threads)
        t.join();
    cout << "counter through increaseCounterByNoLock is " << counter << endl;

    counter = 0;
    for(auto& t : threads)
        t = thread(increaseCounterByLock);
    for(auto& t : threads)
        t.join();
    cout << "counter through increaseCounterByLock is " << counter << endl;

    counter = 0;
    for(auto& t : threads)
        t = thread(increaseCounterByTryLock);
    for(auto& t : threads)
        t.join();
    cout << "counter through increaseCounterByTryLock is " << counter << endl;
}

