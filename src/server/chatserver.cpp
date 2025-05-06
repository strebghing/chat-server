#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include<functional> //注册函数回调时，使用了函数绑定
using namespace std;
using namespace placeholders;
using json=nlohmann::json;


ChatServer::ChatServer(EventLoop *loop, const InetAddress& listenAddr, const string& nameArg)
    :_server(loop,listenAddr,nameArg),  _loop(loop) //将参数传递给类对象，使用类对象调用已实现的muduo库的方法
{
    //注册用户连接的回调
    //setConnectionCallback函数只需传入一个参数（即绑定函数指针）  bind函数需传入三个参数：要绑定的函数指针、当前函数指针、使用占位符表示第三个参数
    _server.setConnectionCallback(bind(&ChatServer::onConnection,this,_1));

    //注册用户事件回调
    _server.setMessageCallback(bind(&ChatServer::onMessage,this,_1,_2,_3));

    //设置线程数量
    _server.setThreadNum(4);
}

void ChatServer::start(){
    _server.start();
}

void ChatServer::onConnection(const TcpConnectionPtr &conn){
    //客户端断开连接 （要包含客户端异常退出的情况）
    if(!conn->connected()){
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

void ChatServer::onMessage(const TcpConnectionPtr& conn, Buffer *buffer, Timestamp time){
    //收到消息
    string buf=buffer->retrieveAllAsString();
    //数据的反序列化
    json js=json::parse(buf);
    
    /*
    收到的信息肯定会有msgid序号和msg_type类型两个属性，不希望直接在网络模块 根据属性去编写判断语句，这是业务模块做的事情，如何让业务模块接收这个属性呢
    使用js[msgid]获取业务模块对应的handle处理器，来获取conn, js, time等在网络模块中接收到的信息属性
    */
   //两句话实现解耦

    //js["msgid"].get<type>() ：将其转成type类型
    auto msgHanlder=ChatService::instance()->getHandler(js["msgid"].get<int>());
    //回调消息绑定好的事件处理器，来执行相应的业务处理
    msgHanlder(conn,js,time);
}

