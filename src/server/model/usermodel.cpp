#include "usermodel.hpp"
#include "db.h"
#include<iostream>
using namespace std;

//User 表的增加方法
bool UserModel::insert(User &user){
    //1. 组装sql语句
    char sql[1024]={0};
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),user.getPassword().c_str(),user.getState().c_str());
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            //获取插入成功的 用户数据的 主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

// User表的查询操作
User UserModel::query(int id){
    //1. 组装sql语句
    char sql[1024]={0};
    sprintf(sql,"select * from User where id=%d",id);
    
    MySQL mysql;
    //连接数据库
    if(mysql.connect()){
        //查询到结果不为空
        MYSQL_RES* res=mysql.query(sql);
        if(res!=nullptr){
            //读取结果的那一行
            MYSQL_ROW row=mysql_fetch_row(res);
            if(row!=nullptr){
                //返回Use信息
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();

}

// 更新数据表中的用户状态信息
bool UserModel::updateState(User &user){
    //1. 组装sql语句
    char sql[1024]={0};
    sprintf(sql,"update User set state='%s' where id=%d",
        user.getState().c_str(),user.getId());
    
    MySQL mysql;
    if(mysql.connect()){
        if(mysql.update(sql)){
            
            return true;
        }
    }

    return false;
}

//重置用户的状态信息
void UserModel::resetState(){
    //1. 组装sql语句
    char sql[1024]={"update User set state='offline' where state='online'"};
    
    MySQL mysql;
    if(mysql.connect()){
        mysql.query(sql);
    }
}