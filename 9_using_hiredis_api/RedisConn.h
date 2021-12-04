#ifndef __REDIS_CONN_H_
#define __REDIS_CONN_H_

#include <iostream>
#include <vector>
#include <map>
#include <list>

#include "hiredis/hiredis.h"

using std::string;
using std::list;
using std::map;
using std::pair;
using std::vector; 

class RedisConn {
public:
	RedisConn() : m_pContext(nullptr) {}
	virtual ~RedisConn();
	
	bool Init(const char* server_ip, int server_port, int db_index, const string password);
	void DeInit();
    /* ------------------- 通用操作 ------------------- */
    bool isExists(string &key); // 判断一个key是否存在
    long del(string &key);	// 删除某个key
	long del(const vector<string>& keys);	// 删除某个key
	bool flushdb();		// 清空数据库

    // ------------------- 字符串相关 -------------------
	string get(string& key);
    bool set(string& key, string& value);
	bool setex(string& key, int timeout, string& value);
	bool setnx(string& key, string& value);
	bool mset(map<string, string>& keys_values);	// 批量设置
    bool mget(const vector<string>& keys, map<string, string>& values);	// 批量获取
	// 原子加减1
    long incr(string key);
    long decr(string key);


	// ---------------- 哈希相关 ------------------------
	string hget(string& key, const string& field);
	long hset(string& key, string& field, string& value);
	long hdel(string& key, const string& field);
	bool hgetAll(string& key, map<string, string>& values);

	// ------------ 链表相关 ------------
	long lpush(string& key, string& value);
	long rpush(string& key, string& value);
	string lpop(string& key);
	string rpop(string& key);
	long llen(string& key);
	bool lrange(string& key, long start, long end, list<string>& values);

protected:
	bool getReplyStatus(redisReply* reply);
	long getReplyInt(redisReply* reply);
	string getReplyString(redisReply* reply);
	bool getReplyMap(redisReply* reply, const vector<string>& keys, map<string, string>& values);	// 用于 mget
	bool getReplyMap(redisReply* reply, map<string, string>& values);	// 用于 hash get
	bool getReplyStringList(redisReply* reply, list<string>& values);

private:
	redisContext* 	m_pContext;
};


#endif