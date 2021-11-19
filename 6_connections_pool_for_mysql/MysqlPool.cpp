#include "MysqlPool.h"
#include <cstring>

#define log_error printf
#define log_warn printf
#define log_info printf

MysqlPool::MysqlPool(string serverIp, uint16_t serverPort, string userName, string password, 
                string dbName, int maxConn) : ConnPool(maxConn)
{
    m_server_ip = serverIp;
    m_server_port = serverPort;
    m_username = userName;
    m_password = password;
    m_db_name = dbName;
}

MysqlConn::MysqlConn(MysqlPool* pool)
{
    m_stmt = NULL;
    m_param_bind = NULL;
    m_param_cnt = 0;
    m_res = NULL;
    m_row = NULL;
    m_pool = pool;
}

MysqlConn::~MysqlConn()
{
    if(m_stmt) 
    {
        mysql_stmt_close(m_stmt);
        m_stmt = nullptr;
    }
    if(m_param_bind)
    {
        delete [] m_param_bind;
        m_param_bind = nullptr;
    }
    if (m_mysql)
	{   // 释放mysql句柄
		mysql_close(m_mysql);
	}
}

int MysqlConn::init()
{
    m_mysql = mysql_init(NULL);	// mysql_前缀为标准的mysql c client的api
    if (!m_mysql)
	{
		log_error("mysql_init failed\n");
		return -1;
	}

    my_bool reconnect = true;
    mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &reconnect);    // mysql_ping实现自动重连
    mysql_options(m_mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");  // 使用utf8mb4编码

    // 连接到服务器
    MYSQL* res = mysql_real_connect(m_mysql, m_pool->getServerIp(), m_pool->getUserName(), m_pool->getpassword(), 
                        m_pool->getDBName(), m_pool->getServerPort(), NULL, 0);
    if(res == NULL)
    {
        log_error("mysql_real_connect failed: %s\n", mysql_error(m_mysql));
		return -1;
    }

    return 0;
}

bool MysqlConn::executeSql(const char* sql)
{
    mysql_ping(m_mysql);
    // 执行了SQL
    int ret = mysql_real_query(m_mysql, sql, strlen(sql));
    if(ret != 0)
    {
        log_error("mysql_real_query failed: %s\n", mysql_error(m_mysql));
		return false;
    }
    return true;
}

bool MysqlConn::executeUpdate(const char* sql)
{
    mysql_ping(m_mysql);

    bool ret =  executeSql(sql);
    if(mysql_affected_rows(m_mysql) > 0)
        return ret;

    return false;
}


bool MysqlConn::initStmtForParam(string &sql)
{
    if(m_stmt) 
    {
        mysql_stmt_close(m_stmt);
        m_stmt = nullptr;
    }
    if(m_param_bind)
    {
        delete [] m_param_bind;
        m_param_bind = nullptr;
    }
    m_param_cnt = 0;

    m_stmt = mysql_stmt_init(m_mysql);
    if(!m_stmt)
    {
        log_error("mysql_stmt_init failed\n");
        return false;
    }

    int ret = mysql_stmt_prepare(m_stmt, sql.c_str(), sql.size());
    if(ret != 0)
    {
        log_error("mysql_stmt_prepare failed: %s\n", mysql_stmt_error(m_stmt));
		return false;
    }

    m_param_cnt = mysql_stmt_param_count(m_stmt);
    if(m_param_cnt > 0)
    {
        m_param_bind = new MYSQL_BIND[m_param_cnt];
        if (!m_param_bind)
		{
			log_error("new failed\n");
			return false;
		}
        memset(m_param_bind, 0, sizeof(MYSQL_BIND) * m_param_cnt);
    }

    return true;
}

bool MysqlConn::executeInsert()
{
    mysql_ping(m_mysql);

    if(!m_stmt)
    {
        log_error("m_stmt not init yet.\n");
        return false;
    }

    my_bool ret = mysql_stmt_bind_param(m_stmt, m_param_bind);
    if(ret != 0)
    {
		log_error("mysql_stmt_bind_param failed: %s\n", mysql_stmt_error(m_stmt));
		return false;
	}

    ret = mysql_stmt_execute(m_stmt);
    if(ret != 0)
    {
		log_error("mysql_stmt_execute failed: %s\n", mysql_stmt_error(m_stmt));
		return false;
	}

    if(mysql_stmt_affected_rows(m_stmt) <= 0)
    {
		log_error("ExecuteUpdate have no effect\n");
		return false;
	}
    return true;
}

bool MysqlConn::executeQuery(const char* sql)
{
    mysql_ping(m_mysql);

    int ret = mysql_real_query(m_mysql, sql, strlen(sql));
    if(ret != 0)
    {
        log_error("mysql_real_query failed: %s, sql: %s\n", mysql_error(m_mysql), sql);
		return false;
    }

    // 保存查询返回的结果
    m_res = mysql_store_result(m_mysql);
    if (!m_res)
	{
		log_error("mysql_store_result failed: %s\n", mysql_error(m_mysql));
		return NULL;
	}

    m_field_idx_map.clear();
    // 将结果的列名与对应的列索引成对保存起来以便于基于列名查询值
    int fields_num = mysql_num_fields(m_res);
    MYSQL_FIELD *fields = mysql_fetch_fields(m_res);
    for(int i = 0; i < fields_num; i++)
    {
        m_field_idx_map.insert(make_pair(fields[i].name, i));
    }

    return true;
}

bool MysqlConn::nextRow()
{
    m_row = mysql_fetch_row(m_res);
    if(m_row) return true;
    else return false;
}

char* MysqlConn::getStringValue(const char* key)
{
    map<string, int>::iterator it = m_field_idx_map.find(key);
    if(it == m_field_idx_map.end()) // 没有找到这个列名
        return nullptr;
    int fIdx = it->second;
    
    MYSQL_FIELD *fields = mysql_fetch_fields(m_res);
    return m_row[fIdx];
}

int MysqlConn::getIntValue(const char* key)
{
    map<string, int>::iterator it = m_field_idx_map.find(key);
    if(it == m_field_idx_map.end()) // 没有找到这个列名
        return 0;
    int fIdx = it->second;

    MYSQL_FIELD *fields = mysql_fetch_fields(m_res);
    return atoi(m_row[fIdx]);
}

void MysqlConn::setstmtParam(uint32_t index, const string& value)
{
    if(index >= m_param_cnt)
    {
        log_error("index[%d] >= m_param_cnt[%d] error...\n", index, m_param_cnt);
        return;
    }
    m_param_bind[index].buffer_type = MYSQL_TYPE_STRING;
    m_param_bind[index].buffer = (char*)value.c_str();
    m_param_bind[index].buffer_length = value.size();
}

void MysqlConn::setstmtParam(uint32_t index, uint32_t& value)
{
    if(index >= m_param_cnt)
    {
        log_error("index[%d] >= m_param_cnt[%d] error...\n", index, m_param_cnt);
        return;
    }
    m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
    m_param_bind[index].buffer = &value;
}

void MysqlConn::setstmtParam(uint32_t index, int& value)
{
    if(index >= m_param_cnt)
    {
        log_error("index[%d] >= m_param_cnt[%d] error...\n", index, m_param_cnt);
        return;
    }
    m_param_bind[index].buffer_type = MYSQL_TYPE_LONG;
    m_param_bind[index].buffer = &value;
}

uint32_t MysqlConn::getStmtLastInsertedId()
{
    return mysql_stmt_insert_id(m_stmt);
}