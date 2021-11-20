#include "ThreadPool.h"
#include <iostream>
#include <string>
using namespace std;

struct workdata
{
    int worknum;
    int waittime;
};
void* work(void* arg)   // 任务函数
{
    workdata* data = (workdata*)arg;
    if(!data)
        return NULL;
    cout << "work[id:" << data->worknum << "] run "<< data->waittime <<"s...\n";
    this_thread::sleep_for(chrono::seconds(data->waittime));  // 用休眠来模拟不同任务函数的不同执行时间
    delete data;
    return NULL;
}

int main()
{
    ThreadPool pool(20);
    pool.init();

    for(int i = 0; i < 10 ; i++)    // 放入10个任务，每个的运行时间不一致
    {
        workdata* data = new workdata{i + 1, i + 1};
        string name = "work";
        pool.acceptATask(work, data, name);
    }

    pool.waitForAllRuningTaskDone();    // 等待任务队列中所有任务都被取走
    cout << "all task done.\n";
    return 0;
}