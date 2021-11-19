#include "ConnPool.h"

#define log_error printf
#define log_warn printf
#define log_info printf

ConnPool::ConnPool(int maxConn) : min_conn(2)
{
    m_max_conn = maxConn;
    m_cur_conn = 0;
    m_abort = false;
}

ConnPool::~ConnPool()
{
    lock_guard<mutex> lock(m_mutex);
    m_abort = true;
    m_cond.notify_all();    // 通知所有正在使用的连接需要终止
    for(Conn* conn : m_free_list)
        delete conn;
    m_free_list.clear();
}

int ConnPool::init()
{
    lock_guard<mutex> lock(m_mutex);
    /* 初始化最小的备用连接数量 */
    for(int i = 0; i < min_conn; i++)
    {
        Conn* pFreeConn = newFreeConn();
        int ret = pFreeConn->init();    // 初始化连接，主要是连接到数据库服务器
        if(ret) // 初始化连接失败
        {
            delete pFreeConn;
            return ret;
        }
        // 初始化连接成功则将其加入到free链表
        m_free_list.emplace_back(pFreeConn);
        m_cur_conn++;
        log_info("pool produce a new conn, now [%d]\n", m_cur_conn);
    }

    return 0;
}

/* timeout_ms默认为-1允许永久阻塞等待
 * timeout_ms >=0 则为等待的超时时间
 */
Conn* ConnPool::takeConn(const int timeout_ms)
{
    unique_lock<mutex> lock(m_mutex);

    /* 向连接池申请连接首先要确保 有空闲的连接 */
    if(m_free_list.empty())
    {
        /* 如果暂时空闲的连接可以用 */
        // 尝试新建连接，如果新建连接数还未达到最大的话
        if(m_cur_conn >= m_max_conn)
        {   // 已达到最大允许连接数，不能再新建连接了，看是否需要超时等待
            auto condition = [this]{return ( !m_free_list.empty()) | m_abort; };
            if(timeout_ms < 0)
            {   // -1 表示可永久阻塞等待
                log_info("wait forever[%d]\n", timeout_ms);
                m_cond.wait(lock, condition);
            }
            else
            {   // 等待一定的时间如果还没有可用的连接就直接返回错误
                log_info("wait [%d]ms\n", timeout_ms);
                bool res = m_cond.wait_for(lock, chrono::milliseconds(timeout_ms), condition);
                if(res == false)
                    return nullptr; // 超时未能获得连接使用权
            }
        }
        else
        {   // 还可以新建连接 则直接增加连接
            Conn* pFreeConn = newFreeConn();
            int ret = pFreeConn->init();    // 初始化连接，主要是连接到数据库服务器
            if(ret) // 初始化连接失败
            {
                delete pFreeConn;
                return nullptr;
            }
            // 初始化连接成功则将其加入到free链表
            m_free_list.emplace_back(pFreeConn);
            m_cur_conn++;
        }
    }

    if(m_abort)
    {
        log_warn("pool aborted!\n");
        return nullptr;
    }

    // 从 free 链表取出一个连接，放入 busy 链表并返回该连接供用户使用
    Conn* pConn = m_free_list.front();
    m_free_list.pop_front();
    m_busy_list.emplace_back(pConn);
    log_info("take a conn. now free[%lu], busy[%lu], total[%lu]\n", m_free_list.size(), m_busy_list.size(), 
                                                            m_free_list.size() + m_busy_list.size());
    return pConn;
}

void ConnPool::putConn(Conn* pConn)
{
    lock_guard<mutex> lock(m_mutex);

    /* 确保要归还的连接在 m_busy_list 中 */
    bool connInBusy = false;
    for(Conn* busyConn : m_busy_list)
    {
        if(busyConn == pConn)
        {
            m_busy_list.remove(pConn);
            m_free_list.emplace_back(pConn);
            m_cond.notify_one();
            log_info("put ok. now free[%lu], busy[%lu], total[%lu]\n", m_free_list.size(), m_busy_list.size(), 
                                                            m_free_list.size() + m_busy_list.size());
            return ;
        }
    }

    log_error("no such conn in busy list.\n");
}

