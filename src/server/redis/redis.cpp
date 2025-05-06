#include"redis.hpp"
#include<iostream>
#include<cerrno>
using namespace std;

Redis::Redis():_publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis(){
    if(_publish_context!=nullptr) redisFree(_publish_context);
    if(_subscribe_context!=nullptr) redisFree(_subscribe_context);
}

bool Redis::connect(){
    //负责publish发布消息的上下文链接
    _publish_context=redisConnect("127.0.0.1",6379);
    if(_publish_context==nullptr){
        cerr<<"connect redis failed!"<<endl;
        return false;
    }

    //负责subscribe订阅消息的上下文链接
    _subscribe_context=redisConnect("127.0.0.1",6379);
    if(_subscribe_context==nullptr){
        cerr<<"connect redis failed"<<endl;
        return false;
    }

    //在单独线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout<<"connect redis-server success!"<<endl;
    return true;
}

//向redis指定的通道channel发布消息
bool Redis::publish(int channel, string msg){
    /*
    redisCommand()函数调用顺序: redisAppendCommand()-->redisBufferWrite()-->redisGetReply 依次调用
    将命令append到本地缓存，从本地缓存中读取命令，等待命令执行结果的返回
    publish命令调用该函数，因为结果返回是紧跟着的，所以不会阻塞线程
    subscribe命令不能直接调用该函数，因为它是阻塞等待返回的
    */
    redisReply *reply=(redisReply*)redisCommand(_publish_context,"PUBLISH %d %s",channel,msg);
    if(reply==nullptr){
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel){
    /*
    subscribe命令本身会造成 线程阻塞 等待通道里发布的信息消息，这里只做订阅，不接收通道消息
    通道消息的接收命令专门在observer_channel_message函数中的独立线程进行；（使用了单例模式）
    只负责发送命令，不阻塞接收redis-server响应的消息，否则和notifyMsg线程抢占资源
    */
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"SUBSCRIBE %d",channel)){
        cerr<<"subscribe command failed!"<<endl;
        return false;
    }
    int done=0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context,&done)==REDIS_ERR){
            cerr<<"subscribe commmand failed!"<<endl;
            return false;
        }
    }
    return true;
}

bool Redis::unsubscribe(int channel){
    if(REDIS_ERR==redisAppendCommand(this->_subscribe_context,"UNSUBSCRIBE %d",channel)){
        cerr<<"unsubscribe command failed!"<<endl;
        return false;
    }
    int done=0;
    while(!done){
        if(redisBufferWrite(this->_subscribe_context,&done)==REDIS_ERR){
            cerr<<"unsubscribe commmand failed!"<<endl;
            return false;
        }
    }
    return true;
}

void Redis::observer_channel_message(){
    redisReply *reply=nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_context,(void**)&reply)){
        //reply携带的数据信息："messange" "channel" "hello word(msg str)"
        //element[2]代表"hello word"观察订阅后的通道收到的消息内容
        if(reply!=nullptr && reply->element[2]!=nullptr && reply->element[2]->str!=nullptr){
            //给业务层上报通道上发生的消息 传入通道号和发送的消息参数变量
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
        freeReplyObject(reply);
    }
    cerr<<">>>>>>>>>>>>>> observer_channel_message quit>>>>>>>>>>>>>>>>>>>"<<endl;
}

//
void Redis::init_notify_handler(function<void(int,string)> fn){
    this->_notify_message_handler=fn;
}

