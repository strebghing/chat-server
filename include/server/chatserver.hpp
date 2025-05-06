#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
using namespace muduo;
using namespace muduo::net;

//聊天服务器的主类头文件
class ChatServer{
public:
    //初始化聊天服务器对象（因为没有默认的构造函数，需要自己传入参数进行构造）传入事件指针、监听的地址、起的名字
    ChatServer(EventLoop *loop, const InetAddress& listenAddr, const string& nameArg);

    //启动服务
    void start();
private:
    //类中的成员，用来实现构造函数，访问底层代码功能
    TcpServer _server; //组合的muduo库，实现服务器功能的类对象
    EventLoop*_loop;    //指向事件循环对象的指针

    //上报用户连接信息的回调函数
    void onConnection(const TcpConnectionPtr&);
    //上报用户事件信息的回调函数
    void onMessage(const TcpConnectionPtr&, Buffer *, Timestamp);
};

#endif