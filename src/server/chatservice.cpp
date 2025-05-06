#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <vector>
using namespace muduo;
using namespace std;

// 获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}

// 构造函数：注册消息以及对应的handle回调操作
ChatService::ChatService()
{
    // 在map集合中插入消息类型以及handle回调函数（通过bind绑定函数指针进行回调函数的注册）
    _msgHandleMap.insert({LOGIN_MSG, std::bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandleMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandleMap.insert({REG_MSG, std::bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandleMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandleMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关的事件处理回调注册
    _msgHandleMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandleMap.insert({ADD_GROUP_MSG, bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandleMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    }
}

// 服务器异常，业务重置方法
void ChatService::reset()
{
    // 服务器退出，将用户状态online改为offline
    _userModel.resetState();
}

// 处理登录业务  传入id pwd字段，验证pwd
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 先简单打印一下所作的操作
    //  LOG_INFO << "do login action!";

    int id = js["id"].get<int>();
    string pwd = js["password"];
    User user = _userModel.query(id);
    if (user.getId() == id && user.getPassword() == pwd)
    {
        // 用户已在线，不允许重复登录
        if (user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 1;
            response["errmsg"] = "该账号已经登陆，请重新输入新的账号";
            conn->send(response.dump());
        }
        else
        {
            // 1. 登陆成功，更新用户状态信息state，且记录用户连接信息(互斥访问)
            //{} 锁的作用范围，lock_guard<type> 类有构造和析构函数，相当于上锁和解锁
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            // 登录成功，向redis订阅channel(id)的消息
            _redis.subscribe(id);

            // 登陆成功，更新用户的状态
            user.setState("online");
            _userModel.updateState(user);

            json response;
            // response=json::array();
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 显式初始化所有数组字段
            // response["offlinemsg"] = json::array(); // 默认空数组
            // response["friends"] = json::array();    // 默认空数组
            // response["groups"] = json::array();     // 默认空数组

            // 2. 登陆成功后，查询该用户有没有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                _offlineMsgModel.remove(id);
            }

            // 3. 登录成功后，查询该用户的好友信息并返回
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &u : userVec)
                {
                    json js;
                    js["id"] = u.getId();
                    js["name"] = u.getName();
                    js["state"] = u.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 4. 登陆成功后，查询该用户的群组信息并返回
            vector<Group> groupVec = _groupModel.queryGroup(id);
            if (!groupVec.empty())
            {
                vector<string> vec3;
                for (Group &g : groupVec)
                {
                    json gjs;
                    gjs["id"] = g.getId();
                    gjs["groupname"] = g.getName();
                    gjs["groupdesc"] = g.getDesc();
                    vector<string> vec4;
                    for (GroupUser &gu : g.getUsers())
                    {
                        json js;
                        js["id"] = gu.getId();
                        js["name"] = gu.getName();
                        js["state"] = gu.getState();
                        js["role"] = gu.getRole();
                        vec4.push_back(js.dump());
                    }
                    gjs["users"] = vec4;
                    vec3.push_back(gjs.dump());
                }
                response["groups"] = vec3;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 登陆失败，可以再细分用户名不存在还是密码错误 导致登陆失败

        if (user.getId() == id)
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 3;
            response["errmsg"] = "密码错误";
            conn->send(response.dump());
        }
        else
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "用户名不存在！请注册";
            conn->send(response.dump());
        }
        // json response;
        // response["msgid"]=LOGIN_MSG_ACK;
        // response["errno"]=2;
        // response["errmsg"]="用户名不存在！请注册";
        // conn->send(response.dump());
    }
}

// 处理注册业务 ---传输name 和 password字段
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO << "do register action!";

    // 从网络模块获取输入的字段
    string name = js["name"];
    string pwd = js["password"];

    // 将字段插入到数据库中
    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功时，给出响应信息：消息类型--响应消息、错误码标识--0表示没有错误、这条消息的id
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 根据消息id获取对应的handle处理器
MsgHandle ChatService::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的handler
    auto it = _msgHandleMap.find(msgid);
    if (it == _msgHandleMap.end())
    {
        // 如果没有对应的回调函数，返回默认的处理器：空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time)
        {
            LOG_ERROR << "msgid:" << msgid << "can not find handler!"; // 输出错误日志
        };
    }
    else
    {
        return _msgHandleMap[msgid];
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 处理客户端异常退出---1. 将记录用户连接的map中的对应关系删除 2. 修改数据表User中这个id的用户状态offline
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 客户端异常退出的情况，也需要取消订阅通道
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

/*以下业务：没有给客户端返回响应，可参考登陆注册业务去写响应
1. 先去public中定义响应的消息类型
2. 参考之前的response去进行响应
*/

// 一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，服务器主动推送消息给toid用户
            it->second->send(js.dump());
            return;
        }
    }

    // 查找toid是否在线（因为可以不在同一个服务器上登录）
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    // toid不在线，存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务 msgid id friendid
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    Group group(-1, name, desc);
    // 这个createGroup方法是关于AllGroup数据表的操作方法
    if (_groupModel.createGroup(group))
    {
        // 这个addGroup方法是关于GroupUser数据表的
        _groupModel.addGroup(userid, group.getId(), "Creator");
    }
}

// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    // 开启群聊天时会上锁，此时用户要把消息接收了才能下线，要不就不进入群聊天--会存储离线消息
    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {

        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 在线转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查找id是否在线（因为可以不在同一个服务器上登录）
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
                return;
            }
            else{
            // 离线存储群消息
            _offlineMsgModel.insert(id, js.dump());
            }
           
        }
    }
}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubscribeMessage(int userid, string msg){
    lock_guard<mutex> lock(_connMutex);
    auto it=_userConnMap.find(userid);
    if(it!=_userConnMap.end()){
        it->second->send(msg);
        return;
    }
    //存储该用户的离线消息
    _offlineMsgModel.insert(userid,msg);
}