#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include <string>
#include <vector>
using namespace std;

class offlineMsgModel{
public:
    //插入离线消息到表中
    void insert(int userid, string msg);

    //根据id删除表中的离线消息
    void remove(int userid);

    //根据id查询表中的离线消息
    vector<string> query(int userid);

};

#endif