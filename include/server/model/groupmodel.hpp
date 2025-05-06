#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include"group.hpp"
#include<string>
#include<vector>
using namespace std;

//维护群组信息的操作接口方法
class GroupModel{
public:

    //创建群聊：根据group信息创建（群名称以及群描述），群组号自动生成
    bool createGroup(Group &group);
    //加入群聊，传入用户id、群聊id、群聊中的角色
    void addGroup(int userid, int groupid, string role);
    //查询用户所在群组：根据用户id查询返回加入的群聊
    vector<Group> queryGroup(int userid);
    //根据指定的groupid查询群组 用户id列表，除了userid自己，主要用户群聊业务 给群组其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);
};

#endif