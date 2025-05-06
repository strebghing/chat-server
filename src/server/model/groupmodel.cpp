#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &g)
{
    // 1. 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s')", g.getName().c_str(), g.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            g.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}

// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 1. 组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values(%d,%d,'%s')", groupid, userid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {

        mysql.update(sql);
    }
}

// 查询用户所在群组列表
vector<Group> GroupModel::queryGroup(int userid)
{
    /* 1. 先根据userid在groupuser表中查询该用户所属的群组信息
    2. 再根据群组信息，查询属于该群组的所有用户id，并且和user表进行多表联合查询，查出用户的详细信息
    */
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
        GroupUser b on a.id=b.groupid where b.userid=%d",
            userid);

    vector<Group> groupvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group g;
                g.setId(atoi(row[0]));
                g.setName(row[1]);
                g.setDesc(row[2]);

                groupvec.push_back(g);
            }

            mysql_free_result(res);
        }
    }

    // 循环查询某组的所有用户
    for (Group &g : groupvec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a inner join \
            GroupUser b on b.userid=a.id where b.groupid=%d",
                g.getId());

        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setState(row[2]);
                user.setRole(row[3]);                
                g.getUsers().push_back(user);
            }

            mysql_free_result(res);
        }
    }
    return groupvec;
}

//根据指定的groupid查询群组用户id列表
vector<int> GroupModel::queryGroupUsers(int userid,int groupid){
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid=%d and userid != %d",
            groupid,userid);

    vector<int> idvec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);

        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                idvec.push_back(atoi(row[0]));
            }

            mysql_free_result(res);
        }
    }
    return idvec;
}
