// 公共头文件，客户端和服务器端都用到的

#ifndef PUBLIC_H
#define PUBLIC_H

//业务涉及的消息类型
enum EnMsgType{
    LOGIN_MSG=1, //登录消息类型
    LOGIN_MSG_ACK,//登录消息的响应
    LOGINOUT_MSG,//注销消息
    REG_MSG,     //注册消息
    REG_MSG_ACK,//注册消息的响应
    ONE_CHAT_MSG,//聊天消息
    ADD_FRIEND_MSG,//添加好友消息
    CREATE_GROUP_MSG,//创建群组
    ADD_GROUP_MSG,//添加群聊
    GROUP_CHAT_MSG,//群聊天
    
};

#endif