#include"friendmodel.hpp"
#include "db.h"

//添加好友关系
//该业务简单地实现单方面的添加，不需要对方的同意，C++不大适合处理业务，简单实现业务，不详细划分实现
void FriendModel::insert(int userid, int friendid){
    // 1. 组装SQL语句
    char sql[1024]={0};
    sprintf(sql,"insert into Friend values(%d, %d)",userid,friendid);
    //2. 更新数据库
    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//查询好友列表
vector<User> FriendModel::query(int userid){
    char sql[1024]={0};
    //联合User和Friend表查询好友列表：通过匹配friend表中的userid得到friendid， 再通过friendid去匹配User表的id去找姓名和状态
    sprintf(sql,"select a.id,a.name,a.state from User a inner join Friend b on b.friendid=a.id where b.userid=%d",userid);

    vector<User> vec;
    MySQL mysql;
    if(mysql.connect()){
        //查询结果的返回
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr){
            // 多行结果，用循环
            MYSQL_ROW row;
            while ((row=mysql_fetch_row(res))!=nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                //将好友信息加入
                vec.push_back(user);
            }
            
            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}

