#include "RedisConn.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define log_error printf
#define log_info printf

RedisConn::~RedisConn()
{
    DeInit();
}

bool RedisConn::getReplyStatus(redisReply* reply)
{
    if(!reply)
    {
        log_error("getReplyStatus reply NULL\n");
        return false;
    }

    bool ret = false;
    if(reply->type == REDIS_REPLY_STATUS)
    {
        ret = !strncmp(reply->str, "OK", 2);
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyStatus error: %s\n", reply->str);
    }
    return ret;
}
long RedisConn::getReplyInt(redisReply* reply)
{
    if(!reply)
    {
        log_error("getReplyInt reply NULL\n");
        return 0;
    }

    long ret = 0;
    if(reply->type == REDIS_REPLY_INTEGER)
    {
        ret = reply->integer;
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyInt error: %s\n", reply->str);
    }
    return ret;
}
string RedisConn::getReplyString(redisReply* reply)
{
    string value;
    if(!reply)
    {
        log_error("getReplyString reply NULL\n");
        return value;
    }

    if(reply->type == REDIS_REPLY_STRING)
    {
        value.append(reply->str, reply->len);
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyString error: %s\n", reply->str);
    }
    return value;
}

bool RedisConn::getReplyMap(redisReply* reply, map<string, string>& values)
{
    if(!reply)
    {
        log_error("getReplyMap reply NULL\n");
        return false;
    }

    if(reply->type == REDIS_REPLY_ARRAY)
    {
        for(size_t i = 0; i < reply->elements; i += 2)
        {
            string key = getReplyString(reply->element[i]);
            string value = getReplyString(reply->element[i + 1]);
            values[key] = value;
        }

        return true;
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyMap error: %s\n", reply->str);
    }
    return false;
}

bool RedisConn::getReplyMap(redisReply* reply, const vector<string>& keys, map<string, string>& values)
{
    if(!reply)
    {
        log_error("getReplyMap reply NULL\n");
        return false;
    }

    if(reply->type == REDIS_REPLY_ARRAY)
    {
        for(size_t i = 0; i < reply->elements; i++)
        {
            redisReply *reply_element = reply->element[i];
            values[keys[i]] = getReplyString(reply_element);
        }

        return true;
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyMap error: %s\n", reply->str);
    }
    return false;
}

bool RedisConn::getReplyStringList(redisReply* reply, list<string>& values)
{
    if(!reply)
    {
        log_error("getReplyStringList reply NULL\n");
        return false;
    }
    values.clear();

    if(reply->type == REDIS_REPLY_ARRAY)
    {
        for(size_t i = 0; i < reply->elements; i++)
        {
            redisReply *reply_element = reply->element[i];
            values.emplace_back(getReplyString(reply_element));
        }

        return true;
    }
    else if(reply->type == REDIS_REPLY_ERROR)
    {
        log_error("getReplyStringList error: %s\n", reply->str);
    }
    return false;
}

bool RedisConn::Init(const char* server_ip, int server_port, int db_index, const string password)
{
    if(m_pContext)
    {
        return false; // 避免未释放的连接重复初始化
    }

    struct timeval timeout = {1, 0};    // 指定连接超时时间1s
    // 建立连接后使用 redisContext 来保存连接状态。
	// redisContext 在每次操作后会修改其中的 err 和  errstr 字段来表示发生的错误码（大于0）和对应的描述。
    m_pContext = redisConnectWithTimeout(server_ip, server_port, timeout);
    if(!m_pContext)
    {
        log_error("redisConnectWithTimeout failed\n");
        return false;
    }

    if(m_pContext->err)
    {
        log_error("redisConnect failed: %s\n", m_pContext->errstr);
        DeInit();
        return false;
    }

    redisReply* reply;  // redisCommand 的返回保存在 redisReply

    // 如果密码非空，则需要进行验证
    if(password.empty() == false)
    {
        reply = (redisReply*)redisCommand(m_pContext, "AUTH %s", password.c_str());
        bool ret = getReplyStatus(reply);
        if(ret == false)
        {
            freeReplyObject(reply);
            DeInit();
            return ret;
        }
    }

    // 选择数据库
    reply = (redisReply*)redisCommand(m_pContext, "SELECT %d", db_index);
    bool ret = getReplyStatus(reply);
    freeReplyObject(reply);
    return ret;
}

void RedisConn::DeInit()
{
    if (m_pContext)
	{
		redisFree(m_pContext);
		m_pContext = NULL;
	}
}

bool RedisConn::isExists(string &key)
{
    if(!m_pContext)
        return false;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "EXISTS %s", key.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return bool(ret);
}

long RedisConn::del(string &key)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "DEL %s", key.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

long RedisConn::del(const vector<string>& keys)
{
    if(!m_pContext)
        return 0;

    // 构造 del语句
    string mcmd = "DEL";
    for(string key : keys)
    {
        mcmd.append(" ");
        mcmd.append(key);
    }

    redisReply* reply = (redisReply*)redisCommand(m_pContext, mcmd.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

bool RedisConn::flushdb()
{
    if(!m_pContext)
        return false;
    
    redisReply *reply = (redisReply *)redisCommand(m_pContext, "FLUSHDB");
	bool ret = getReplyStatus(reply);
    freeReplyObject(reply);
    return ret;
}

string RedisConn::get(string& key)
{
    string value;
    if(!m_pContext)
        return value;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "GET %s", key.c_str());
    value = getReplyString(reply);
    freeReplyObject(reply);
    return value;
}
bool RedisConn::set(string& key, string& value)
{
    if(!m_pContext)
        return false;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "SET %s %s", key.c_str(), value.c_str());
    bool ret = getReplyStatus(reply);
    freeReplyObject(reply);
    return ret;
}


bool RedisConn::setex(string& key, int seconds, string& value)
{
    if(!m_pContext)
        return false;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "SETEX %s %d %s", key.c_str(), seconds, value.c_str());
    bool ret = getReplyStatus(reply);
    freeReplyObject(reply);
    return ret;
}

bool RedisConn::setnx(string& key, string& value)
{
    if(!m_pContext)
        return false;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "SETNX %s %s", key.c_str(), value.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return bool(ret);
}

bool RedisConn::mset(map<string, string>& keys_values)
{
    if(!m_pContext || keys_values.empty())
        return false;

    // 构造 mset语句
    string mcmd = "MSET";
    for(pair<string, string> key_value : keys_values)
    {
        mcmd.append(" ");
        mcmd.append(key_value.first);
        mcmd.append(" ");
        mcmd.append(key_value.second);
    }
    redisReply* reply = (redisReply*)redisCommand(m_pContext, mcmd.c_str());
    bool ret = getReplyStatus(reply);
    freeReplyObject(reply);
    return ret;
}

bool RedisConn::mget(const vector<string>& keys, map<string, string>& values)
{
    if(!m_pContext || keys.empty())
        return false;

    values.clear();

    // 构造 mget语句
    string mcmd = "MGET";
    for(string key : keys)
    {
        mcmd.append(" ");
        mcmd.append(key);
    }

    redisReply* reply = (redisReply*)redisCommand(m_pContext, mcmd.c_str());
    bool ret = getReplyMap(reply, keys, values);
    freeReplyObject(reply);
    return ret;
}

long RedisConn::incr(string key)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "INCR %s", key.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

long RedisConn::decr(string key)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "DECR %s", key.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}


long RedisConn::lpush(string& key, string& value)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "LPUSH %s %s", key.c_str(), value.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

long RedisConn::rpush(string& key, string& value)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "RPUSH %s %s", key.c_str(), value.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

string RedisConn::lpop(string& key)
{
    string value;
    if(!m_pContext)
        return value;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "LPOP %s", key.c_str());
    value = getReplyString(reply);
    freeReplyObject(reply);
    return value;
}
string RedisConn::rpop(string& key)
{
    string value;
    if(!m_pContext)
        return value;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "RPOP %s", key.c_str());
    value = getReplyString(reply);
    freeReplyObject(reply);
    return value;
}

long RedisConn::llen(string& key)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "LLEN %s", key.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}
bool RedisConn::lrange(string& key, long start, long end, list<string>& values)
{
    if(!m_pContext)
        return false;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "LRANGE %s %ld %ld", key.c_str(), start, end);
    bool ret = getReplyStringList(reply, values);
    freeReplyObject(reply);
    return ret;
}

string RedisConn::hget(string& key, const string& field)
{
    string value;
    if(!m_pContext)
        return value;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "HGET %s %s", key.c_str(), field.c_str());
    value = getReplyString(reply);
    freeReplyObject(reply);
    return value;
}

long RedisConn::hset(string& key, string& field, string& value)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}

long RedisConn::hdel(string& key, const string& field)
{
    if(!m_pContext)
        return 0;

    redisReply* reply = (redisReply*)redisCommand(m_pContext, "HDEL %s %s", key.c_str(), field.c_str());
    long ret = getReplyInt(reply);
    freeReplyObject(reply);
    return ret;
}
bool RedisConn::hgetAll(string& key, map<string, string>& values)
{
    if(!m_pContext)
        return false;

    values.clear();
    redisReply* reply = (redisReply*)redisCommand(m_pContext, "HGETALL %s", key.c_str());
    bool ret = getReplyMap(reply, values);
    freeReplyObject(reply);
    return ret;
}