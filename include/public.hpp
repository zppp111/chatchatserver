/*

server和client端 的公共文件

*/
#ifndef PUBLIC_H
#define PUBLIC_H

enum EnMsgType
{
    /*跟登录业务绑定起来 反序列化后得到1，自动跳转登陆业务*/
    LOGIN_MSG = 1, // 登陆消息
    LOGIN_MSG_ACK, // 登录响应消息

    LOGINOUT_MSG,    //注销消息
    REG_MSG,       // 注册消息
    REG_MSG_ACK,   // 注册响应消息

    ONE_CHAT_MSG ,  // 聊天消息

    ADD_FRIEND_MSG,  //添加好友消息

    CREATE_GROUP_MSG, //创建群组
    ADD_GROUP_MSG,    //加入群组
    GROUP_CHAT_MSG,   //群内聊天

    

};

#endif