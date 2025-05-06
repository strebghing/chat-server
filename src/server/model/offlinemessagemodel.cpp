#include "offlinemessagemodel.hpp"
#include "db.h"

//添加离线消息方法
void offlineMsgModel::insert(int userid, string msg){
    char sql[1024]={0};
    sprintf(sql,"insert into OfflineMessage values(%d, '%s')",userid,msg.c_str());

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }
}

//删除对应id的离线消息
void offlineMsgModel::remove(int userid){
    char sql[1024]={0};
    sprintf(sql,"delete from OfflineMessage where userid=%d",userid);

    MySQL mysql;
    if(mysql.connect()){
        mysql.update(sql);
    }

}

//查询对应id的离线消息
vector<string> offlineMsgModel::query(int userid){
    char sql[1024]={0};
    sprintf(sql,"select message from OfflineMessage where userid=%d",userid);

    //vec用来存储查到的多条离线消息
    vector<string> vec;
    MySQL mysql;  
    //连接数据库
    if(mysql.connect()){
        //调用MySQL的查询API，接收查询结果
        MYSQL_RES* res=mysql.query(sql);
        //查询结果不为空
        if(res!=nullptr){
            //循环的将每一行结果存储起来
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }

            //释放资源空间
            mysql_free_result(res);
            //返回数组
            return vec;
        }
    }
    return vec;
}