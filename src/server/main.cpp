#include "chatserver.hpp"
#include<iostream>
#include<signal.h>
#include"chatservice.hpp"
using namespace std;

//服务器ctrl+c中断，更新用户的状态信息
void resetHandle(int){
    ChatService::instance()->reset();
    exit(0);
}
int main(int argc, char** argv){

    //实现多个ChatServer，与nginx配置的stream中的server端口保持一致，开启多台服务器
    if(argc<3){
        cerr<<"command invalid! example: ./ChatServer 127.0.0.1 6000"<<endl;
        exit(-1);
    }
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);

    signal(SIGINT,resetHandle);
    // 声明需要传入的参数和对象，使用对象调用chatServer类的方法
    EventLoop loop;
    InetAddress addr(ip,port);
    ChatServer server(&loop,addr,"ChatServer");

    server.start();
    loop.loop();

    return 0;
}