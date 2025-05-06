#include "db.h"
#include <muduo/base/Logging.h>

//数据库配置信息
static string server ="127.0.0.1";
static string user="root";
static string password="123456";
static string dbname="chat";

//初始化数据库连接
MySQL::MySQL(){
    _conn=mysql_init(nullptr); //分配资源空间
}

//释放数据库连接资源
MySQL::~MySQL(){
    if(_conn!=nullptr)
        mysql_close(_conn);
}

//连接数据库
bool MySQL::connect(){
    //传入数据库连接、服务器名称、用户名称、数据库密码、数据库名字、连接的端口信息参数
    MYSQL *p=mysql_real_connect(_conn, server.c_str(), user.c_str(), 
                                password.c_str(), dbname.c_str(), 3306,nullptr, 0);
    
    if(p!=nullptr){
        //设置中文显示，因为语法默认ASCII码
        mysql_query(_conn,"set names gbk");
        //输出连接成功提醒
        LOG_INFO<<"数据库连接成功！";
    }
    else{
        LOG_INFO<<"数据库连接失败";
    }
    return p;
}

//更新操作
bool MySQL::update(string sql){
    if(mysql_query(_conn, sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"更新失败！";
        return false;
    }
    return true;
}

//查询操作
MYSQL_RES* MySQL::query(string sql){
    if(mysql_query(_conn, sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"<<sql<<"查询失败！";
        return nullptr;
    }

    return mysql_use_result(_conn);
}

//获取数据库连接
MYSQL* MySQL::getConnection(){
    return _conn;
}


