#ifndef REDIS_HPP
#define REDIS_HPP

#include<hiredis/hiredis.h>
#include<string>
#include<functional>
#include<thread>
using namespace std;

class Redis{
public:
    Redis();
    ~Redis();

    //连接redis服务器
    bool connect();

    //向redis指定的通道发布消息
    bool publish(int channel, string msg);

    //向redis指定的通道订阅消息
    bool subscribe(int channel);

    //向redis指定的通道取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程中，接收订阅通道的消息
    void observer_channel_message();

    //初始化 向业务层上报 通道消息 的回调对象
    void init_notify_handler(function<void(int,string)> fn);

private:
    //hiredis同步上下文对象，负责publish消息
    redisContext* _publish_context;

    //hiredis同步上下文对象，负责subscribe消息
    redisContext* _subscribe_context;

    //回调操作，收到订阅消息，给server层上报
    function<void(int,string)> _notify_message_handler;
    
};

#endif
