#include "MysqlPool.h"
#include <sstream>
#include <thread>
#include <atomic>
#define log_error printf
#define log_warn printf
#define log_info printf

#define DB_HOST_IP          "127.0.0.1"             // 数据库服务器ip
#define DB_HOST_PORT        3306
#define DB_DATABASE_NAME    "mysql_pool_test"       // 数据库对应的库名字, 这里需要自己提前用命令创建db
#define DB_USERNAME         "book"                  // 数据库用户名
#define DB_PASSWORD         "123456"                // 数据库密码
#define DB_POOL_MAX_CON     3                       // 连接池支持的最大连接数量

#define DROP_USER_TABLE	"DROP TABLE IF EXISTS User"

#define CREATE_USER_TABLE "CREATE TABLE User (     \
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT COMMENT '用户id',   \
  `sex` tinyint(1) unsigned NOT NULL DEFAULT '0' COMMENT '1男2女0未知', \
  `name` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '用户名',  \
  `password` varchar(32) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '密码',    \
  `phone` varchar(11) COLLATE utf8mb4_bin NOT NULL DEFAULT '' COMMENT '手机号码',   \
  PRIMARY KEY (`id`),   \
  KEY `idx_name` (`name`),  \
  KEY `idx_phone` (`phone`) \
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_bin;"     

struct user
{
    uint32_t id;
    int sex;
    string name;
    string passwd;
    string phone;
};
static user users[4] = 
{
    {.id = 0, .sex = 1, .name = "二狗", .passwd = "123456",  .phone = "13593937555"},
    {.id = 0, .sex = 2, .name = "三狗", .passwd = "1234aaa", .phone = "13593789555"},
    {.id = 0, .sex = 1, .name = "大傻", .passwd = "ccca@aa", .phone = "18673789885"},
    {.id = 0, .sex = 2, .name = "二傻", .passwd = "sssfff4", .phone = "18905449355"},
};
static atomic<int> userIdx(0);
static atomic<bool> tableBuilt(false);

static string int2string(uint32_t user_id)
{
    stringstream ss;
    ss << user_id;
    return ss.str();
}

void queryTest(MysqlConn* myconn)
{
    int idx = (userIdx++)%4;
    string sql = "select * from User where id=" + int2string(idx);
    bool ret = myconn->executeQuery(sql.c_str());
    if(ret)
    {
        if(myconn->nextRow())
        {
            user u;
            u.id = myconn->getIntValue("id");
            u.sex = myconn->getIntValue("sex");
            u.name = myconn->getStringValue("name");
            u.phone = myconn->getStringValue("phone");

            printf("-------------------------------------------\n");
            printf("%s\t%s\t%s\t%s\n", "id", "sex", "name", "phone");
            printf("%d\t%s\t%s\t%s\n", u.id, (u.sex == 1)?"男":"女", u.name.c_str(), u.phone.c_str());
        }
    }
}

void updateTest(MysqlConn* myconn)
{
    int idx = (userIdx++)%4;
    string sql = "update User set `sex`=" + int2string(users[idx].sex) + ",`name`='" + users[idx].name + \
                    "',`phone`='" + users[idx].phone + "' where id=" + int2string(idx);
    log_info("go update[%d]\n",idx);
    myconn->executeUpdate(sql.c_str());
}

void insertTest(MysqlConn* myconn)
{
    string sql = "insert into User(`sex`,`name`,`password`,`phone`) values(?,?,?,?)";
    bool ret = myconn->initStmtForParam(sql);
    if(!ret)
        return ;
    int index = 0;
    int idx = (userIdx++)%4;
    myconn->setstmtParam(index++, users[idx].sex);
    myconn->setstmtParam(index++, users[idx].name);
    myconn->setstmtParam(index++, users[idx].passwd);
    myconn->setstmtParam(index++, users[idx].phone);

    ret = myconn->executeInsert();
    if(ret)
    {
        users[idx].id = myconn->getStmtLastInsertedId();
        log_info("executeInsert[id=%d] ok.\n", users[idx].id);
    }
    else
    {
        log_error("executeInsert failed\n");
    }
}

void mysqlCrudTest(MysqlPool* mypool, int sleepSec)
{
    MysqlConn* myconn = mypool->takeConn(2000);
    if(myconn)
    {
        myconn->init();
        if(!tableBuilt.exchange(true))  // 由第一个走到这里的线程去创建表
        {
            myconn->executeSql(DROP_USER_TABLE);
            myconn->executeSql(CREATE_USER_TABLE);
        }
        else this_thread::sleep_for(chrono::milliseconds(100)); // 这里做简单处理，让其他线程延时一下再进行操作

        insertTest(myconn);
        queryTest(myconn);
        updateTest(myconn);
        queryTest(myconn);

        this_thread::sleep_for(chrono::seconds(sleepSec)); // 休眠2秒再返回连接
        mypool->putConn(myconn);
    }
    else
    {
        log_error("failed to get a MysqlConn\n");
    }
}

int main()
{
    MysqlPool mypool(DB_HOST_IP, DB_HOST_PORT, DB_USERNAME, DB_PASSWORD, DB_DATABASE_NAME, DB_POOL_MAX_CON);
    mypool.init();

    thread t1(mysqlCrudTest, &mypool, 2);
    thread t2(mysqlCrudTest, &mypool, 2);
    thread t3(mysqlCrudTest, &mypool, 1);
    thread t4(mysqlCrudTest, &mypool, 1);

    t1.join();
    t2.join();  
    t3.join();
    t4.join();
    return 0;
}