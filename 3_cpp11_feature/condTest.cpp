#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <list>

using namespace std;


template<typename T>
class SyncQueue
{
public:
    SyncQueue() {}
    ~SyncQueue() {}

    void Put(const T & one)
    {
        unique_lock<mutex> locker(_mutex);
        _queue.emplace_back(one);
        _notEmpty.notify_one();
    }

    T & Take()
    {
        unique_lock<mutex> locker(_mutex);
        _notEmpty.wait(locker, [this](){ return !_queue.empty(); });

        T& one = _queue.front();
        _queue.pop_front();

        return one;
    }

    bool isEmpty()
    {
        unique_lock<mutex> locker(_mutex);

        return _queue.empty();
    }

    size_t Size()
    {
        unique_lock<mutex> locker(_mutex);

        return _queue.size();
    }

private:
    list<T> _queue;
    mutex _mutex;
    condition_variable _notEmpty;
};


SyncQueue<int> sq;

void PutData()
{
    for(int i = 0; i < 20; i++)
    {
        sq.Put(i);
        cout << "# sq put " << i << endl;
    }
}

void TakeData()
{
    for(int i = 0; i < 20; i++)
    {
        int one = sq.Take();
        cout << "# sq take " << one << endl;
    }
}

int main()
{
    thread t1(PutData);
    thread t2(TakeData);

    t1.join();
    t2.join();
    
}