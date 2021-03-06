#include <cstring>
#include "ThreadPool.h"

using namespace std;
#define log_error printf
#define log_warn printf
#define log_info printf

Task::Task(TaskCallback cb, void *args, string& name)
{
    m_args = args;
    m_callback = cb;
    m_name = name;
}

ThreadPool::~ThreadPool()
{
    unique_lock<mutex> lock(m_mutex);

    for(Thread* t : m_thd_list)
        t->setToTerminate();
    m_cond.notify_all();
    // 等待所有线程终止
    m_cond.wait(lock, [this]{ return m_thd_list.empty(); });
    // 清空任务队列
    for(Task* task : m_task_list)
        delete task;
    m_task_list.clear();
}

// 此函数内不加锁，外部调用加锁
Thread* ThreadPool::createAThread()
{
    Thread* pth = new Thread(this);
    if(pth)
    {
        thread t(thread_function, pth);
        pth->setTid(t.get_id());
        t.detach();
        m_thd_list.emplace_back(pth);
        log_info("a new thread[%lu] created. total[%lu]\n", pth->getTid(), m_thd_list.size());
    }
    return pth;
}

/* 初始化过程主要是预先起最小线程数作为备用 */
bool ThreadPool::init()
{
    lock_guard<mutex> lock(m_mutex);
    for(int i = 0; i < m_min_thd_num; i++)
    {
        Thread* pth = createAThread();
        if(!pth)
            return false;
    }

    return true;
}

/* 这个函数在比较清闲的时候将执行队列中的线程减少一半，但不小于最小备用数量 */
// 假定条件为：任务队列空了，且没有线程处于忙状态，则减少线程，仅做测试
int ThreadPool::terminateSomeThread()
{   // 此函数调用者加锁，内部不加锁
    int terminateNum = 0;
    if(m_task_list.empty() && m_busy_thd_num == 0)   
    {
        int needThreadNum = m_thd_list.size() >> 1; // 线程数减半
        needThreadNum = max(needThreadNum, m_min_thd_num); // 但必须大于最小备用线程数
        terminateNum = m_thd_list.size() - needThreadNum;
        int i = 0;
        for(auto it = m_thd_list.begin(); i < terminateNum && it != m_thd_list.end(); i++, it++)
        {
            Thread* t = *it;
            t->setToTerminate();    // 通知该线程终止
        }
    }
    return terminateNum;
}

/* 这个函数在任务队列增长但没有更多线程可用时将执行队列中的线程数量扩大一半，但不大于最大允许数量 */
// 假定条件为：如果所有线程目前都处于忙的状态，但任务队列中的等待任务数却超过当前线程数的一半，则需要增加增加线程，仅做测试
int ThreadPool::activeSomeThread()
{   // 此函数调用者加锁，内部不加锁
    int needThreadNum = 0;
    if(m_busy_thd_num >= m_thd_list.size() && 
            m_task_list.size() >= (m_thd_list.size() >> 1))
    {
        needThreadNum = m_thd_list.size() << 1; // 线程数翻倍
        needThreadNum = min(needThreadNum, m_max_thd_num); // 但必须小于最大允许线程数
        needThreadNum = needThreadNum - m_thd_list.size(); // 最终实际增加的线程数

        for(int i = 0; i < needThreadNum; i++)
        {
            Thread* pth = createAThread();
            if(!pth)
                return 0;
        }
    }
    return needThreadNum;
}

int ThreadPool::acceptATask(TaskCallback cb, void* args, string& taskName)
{
    Task *task = new Task(cb, args, taskName);
    
    lock_guard<mutex> lock(m_mutex);
    m_task_list.emplace_back(task);
    m_cond.notify_one();    // 通知任意一个就绪的线程
    log_info("new task[%s] accepted. now[%lu]\n", taskName.c_str(), m_task_list.size());
    return 0;
}

bool ThreadPool::waitForAllRuningTaskDone()
{
    unique_lock<mutex> lock(m_notask_mutex);
    m_notask_cond.wait(lock);
    return true;
}

/* 通过传参使一个Thread实例绑定到了一个 thread_function函数，
 * thread_function 需要: 1）为该实例“争取到”任务去执行（即从任务队列中取任务实例，执行完毕后释放任务实例）；
                         2）在该实例被线程池指定必须终止时，从执行队列删除该线程实例并使线程退出
                         3）在适当的时机放缩线程池
*/
void thread_function(Thread* t)
{
    ThreadPool* pool = t->m_pool;
    unique_lock<mutex> lock(pool->m_mutex);

    do
    {
        pool->m_cond.wait(lock, [pool, t]{ return !pool->m_task_list.empty() || t->m_needToTerminate; });
        
        if(t->m_needToTerminate) // 线程需要终止
        {
            pool->m_thd_list.remove(t);
            log_info("thread[%lu] is terminated. now[%lu]\n", t->getTid(), pool->m_thd_list.size());
            delete t;
            if(pool->m_thd_list.size() == 0)
            {   // 所有线程都被销毁了，这只可能发生在销毁整个线程池的情况下
                pool->m_cond.notify_all(); // 通知线程池析构函数执行队列中所有线程都已销毁，可完成析构
            }
            return ; // 此处直接退出， unique_lock不需要手动解锁
        }

        /* 当前线程从任务队列中取任务 */
        Task* task = pool->m_task_list.front();
        pool->m_task_list.pop_front();
        pool->m_busy_thd_num++; // 一个线程开始忙了

        /* 这里判断是否需要增加更多线程 */
        pool->activeSomeThread();

        lock.unlock();  // 运行任务前先解锁
        log_info("a task[%s] was take by [%lu]. now [%lu]task left\n", 
                    task->getName().c_str(), t->getTid(), pool->m_task_list.size());
        task->Run();
        log_info("thread[%lu] done the task.\n", t->getTid());
        delete task;    // 任务执行完要记得释放
        lock.lock();    // 再加锁进入下一次循环等待
        pool->m_busy_thd_num--; // 有一个线程解放了，去接下一个任务

        /* 这里判断是否需要减少线程 */
        pool->terminateSomeThread();
        
        if(pool->m_task_list.empty())
        {
            pool->m_notask_cond.notify_all();// 通知用户或线程池任务队列空了
        }
    } while (true);
}