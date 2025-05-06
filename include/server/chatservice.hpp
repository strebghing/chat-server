#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
using namespace std;
using namespace muduo;
using namespace muduo::net;

#include "json.hpp"
using json=nlohmann::json;

#include"usermodel.hpp"
#include<mutex>
#include "offlinemessagemodel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"

using MsgHandle=std::function<void(const TcpConnectionPtr& conn, json &js, Timestamp time)>; //回调函数

//聊天服务器业务类
class ChatService{
    public:
    //获取单例对象的接口函数
    static ChatService* instance();

    //处理登录业务
    void login(const TcpConnectionPtr&conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr&conn, json& js, Timestamp time);
    //处理一对一聊天业务
    void oneChat(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //加入群组业务
    void addGroup(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //群聊天业务
    void groupChat(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //处理注销业务
    void loginout(const TcpConnectionPtr& conn,json &js,Timestamp time);
    //获取消息对应的处理器handle
    MsgHandle getHandler(int msgid);  
    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //服务器异常，业务重置
    void reset();
    //处理redis上报消息业务
    void handleRedisSubscribeMessage(int channel, string msg);
    

    private:
    //构造函数
    ChatService();

    //存储消息id和其对应的业务处理方法，这个业务处理不会随着用户连接增加而动态改变
    unordered_map<int,MsgHandle> _msgHandleMap;

    //数据操作类对象
    UserModel _userModel;
    offlineMsgModel _offlineMsgModel;
    FriendModel _friendModel;
    GroupModel _groupModel;


    //存储在线用户的通信连接（因为当有用户发消息时，服务器需要将这条消息给目标用户，会用到目标用户的连接_conn）
    //需要注意线程安全：同一时间多用户都可以往map里记录连接，动态变化
    unordered_map<int ,TcpConnectionPtr> _userConnMap;
    //定义互斥锁，保证_userConnMap的安全
    mutex _connMutex;

    //redis操作对象
    Redis _redis;

};

#endif