#ifndef DB_H
#define DB_H

#include<mysql/mysql.h>
#include<string>
using namespace std;

//数据库操作类
class MySQL{
    public:
    //初始化数据库连接
    MySQL();

    //释放数据库连接资源
    ~MySQL();

    //连接数据库
    bool connect();

    //更新操作
    bool update(string sql);

    //查询操作
    MYSQL_RES* query(string sql);

    //获取数据库连接这个私有成员变量，提供给外部去调用 数据库连接变量能够指向的方法 _conn->func1()
    MYSQL* getConnection();

    private:
    MYSQL *_conn;
};

#endif