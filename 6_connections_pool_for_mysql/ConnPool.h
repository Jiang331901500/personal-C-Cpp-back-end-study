#ifndef __CONN_POOL_H_
#define __CONN_POOL_H_

#include <string>
#include <list>
#include <mutex>
#include <condition_variable>
#include <map>
#include <iostream>
#include <stdint.h>

using namespace std;

/* 连接类接口 */
class Conn
{
public:
    virtual ~Conn() {};
    virtual int init() = 0;
};

/* 连接池类接口 */
class ConnPool
{
public:
    ConnPool() : min_conn(2), m_max_conn(5), m_cur_conn(0), m_abort(false) {}
    ConnPool(int maxConn = 5);  // 初始化只需设置最大连接数
    virtual ~ConnPool();

    virtual int init();     // 使用池之前总是初始化这个池
    virtual Conn* takeConn(const int timeout_ms = -1);  // 从池中拿一个连接
    virtual void putConn(Conn* pConn);  // 将一个连接返回池中

protected:
    virtual Conn* newFreeConn() = 0;  // 根据具体的连接库类型去实现

    const int   min_conn;       // 最小备用连接数量
	int			m_cur_conn;	    // 当前已创建的连接数量
	int 		m_max_conn;	    // 最大允许创建连接数量  
	list<Conn*>	m_free_list;	// 空闲的连接
	list<Conn*>	m_busy_list;    // 正在被使用的连接

    mutex m_mutex;
    condition_variable m_cond;
    bool m_abort;   // 终止信号
};

#endif