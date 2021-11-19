#ifndef __MYSQL_CONN_POOL_H_
#define __MYSQL_CONN_POOL_H_

#include "ConnPool.h"
#include <mysql/mysql.h>

class MysqlPool;
class MysqlConn : public Conn
{
public:
    MysqlConn(MysqlPool* pool);
    virtual int init(); // 初始化连接，即建立连接到mysql服务器
    virtual ~MysqlConn();

    /* 数据库对象操作 */
	// 执行SQL语句
	bool executeSql(const char* sql); // 执行任意SQL语句，不查看返回结果和影响行数，否则用下面的函数
    bool executeInsert(/* through MYSQL_STMT */); // 插入一行新数据，需要先执行 initANewStatement()并传入参数
    bool executeUpdate(const char* sql); // 执行更新操作的sql
    bool executeQuery(const char* sql); // 执行查询操作，结果保存在 m_res，通过 nextRow()遍历每一行，用getValue()获取值

    /* 用户接口辅助方法 */
    bool initStmtForParam(string &sql);   // 初始化一个 statement，如果已有初始化过的 statement，将会被重置

    //bool initStmtForResult(string &sql);  // TODO
    bool nextRow();     // 取下一列，如果有的话
    char* getStringValue(const char* key); // 输入列名，获取当前行该列的值
    int getIntValue(const char* key);

	void setstmtParam(uint32_t index, uint32_t& value);    // setstmtParam 负责设置 statement 中的各个参数的值
    void setstmtParam(uint32_t index, int& value);
    void setstmtParam(uint32_t index, const string& value);
    uint32_t getStmtLastInsertedId();   // 最后一次插入记录的id

private:
    // mysql句柄，对应一个连接
    MYSQL* 		m_mysql;
     // 提交 sql前构建 statment
    MYSQL_STMT*	m_stmt;
	MYSQL_BIND*	m_param_bind;
	uint32_t	m_param_cnt;
    // 返回查询结果时使用
    MYSQL_RES* 			m_res;
	MYSQL_ROW			m_row;
    // 保存当前返回的查询结果中列名的索引
	map<string, int>	m_field_idx_map;

    MysqlPool* m_pool;  // 指向所在的连接池
};

class MysqlPool : public ConnPool
{
public:
    MysqlPool(string serverIp, uint16_t serverPort, string userName, string password, 
                string dbName, int maxConn = 5);
    ~MysqlPool() {}
    const char* getServerIp() {return m_server_ip.c_str();}
    uint16_t getServerPort() {return m_server_port;}
    const char* getUserName() {return m_username.c_str();}
    const char* getpassword() {return m_password.c_str();}
    const char* getDBName() {return m_db_name.c_str();}

    virtual MysqlConn* takeConn(const int timeout_ms) { return static_cast<MysqlConn*>(ConnPool::takeConn(timeout_ms)); }

private:
    virtual MysqlConn* newFreeConn() { return new MysqlConn(this);}

    string 		m_server_ip;	// 数据库ip
	uint16_t	m_server_port;  // 数据库端口
	string 		m_username;  	// 用户名
	string 		m_password;		// 用户密码
	string 		m_db_name;		// db名称
};

#endif
