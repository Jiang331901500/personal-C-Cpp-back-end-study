#include "RedisConn.h"
#include <iostream>
#include <string>
using namespace std;

#define DB_HOST_IP          "192.168.0.105"       // 数据库服务器ip
#define DB_HOST_PORT        6379
#define DB_INDEX            0                       // redis默认支持16个db
#define DB_PASSWORD         "123456"                // 数据库密码，不设置AUTH时该参数为空

int main()
{
    RedisConn rc;
    rc.Init(DB_HOST_IP, DB_HOST_PORT, DB_INDEX, DB_PASSWORD);

    /* 测试 string */
    cout << "### test set, get, incr, decr and del\n";
    string key = "akey";
    string value = "1";
    rc.set(key, value);
    cout << key << " : " << rc.get(key) << endl;
    rc.incr(key);
    cout << key << " : " << rc.get(key) << endl;
    rc.decr(key);
    cout << key << " : " << rc.get(key) << endl;
    rc.del(key);

    cout << "\n### test mset, mget\n";
    vector<string> keys = {"f1001", "f1002", "f1003"};
    map<string, string> values;
    values["f1001"] = "v1001";
    values["f1002"] = "v1002";
    values["f1003"] = "v1003";
    rc.mset(values);
    rc.mget(keys, values);  // 内部会将 values 清空
    for(pair<string, string> kv : values)
    {
        cout << kv.first << " : " << kv.second << endl;
    }
    rc.del(keys);

    /* 测试 list */
    cout << "\n### test list operation\n";
    string list_key = "mylist";
    list<string> vlist = {"1001", "2002", "3003", "4004"};
    for(string v : vlist)
    {
        rc.lpush(list_key, v);
    }
    for(string v : vlist)
    {
        rc.rpush(list_key, v);
    }
    rc.lrange(list_key, 0, -1, vlist);
    cout << "[";
    for(string v : vlist)
    {
        cout << v << ", ";
    }
    cout << "]\n";
    cout << "rpop: " << rc.rpop(list_key) << endl;
    cout << "lpop: " << rc.lpop(list_key) << endl;
    rc.del(list_key);

    /* 测试 hash */
    cout << "\n### test hash operation\n";
    string hash_key = "myhash";
    values.clear();
    values["f1001"] = "v1001";
    values["f1002"] = "v1002";
    values["f1003"] = "v1003";
    for(pair<string, string> v : values)
    {
        rc.hset(hash_key, v.first, v.second);
    }
    cout << "hget first: " << rc.hget(hash_key, values.begin()->first) << endl;
    rc.hdel(hash_key, values.begin()->first);
    rc.hgetAll(hash_key, values);
    for(pair<string, string> v : values)
    {
        cout << v.first << " : " << v.second << endl;
    }
    rc.del(hash_key);

    rc.DeInit();
    return 0;
}