/*I
网络模块搭建 ---muduo库
基于事件驱动 IO复用  EPOLL多线程池的网络代码
基于reactor模型

*/

#ifndef CHATSERVER_H  //C++头文件
#define CHATSERVER_H

#include <muduo/net/TcpServer.h>
#include <muduo/net/EventLoop.h>
using namespace muduo; 
using namespace muduo::net; //不想写那么长的名字空间作用域 提前定义

// 聊天服务器的类
class ChatServer

{
    // 初始化服务器对象
public:
    ChatServer(EventLoop *loop,
               const InetAddress &listenAddr,
               const string &nameArg );

    // 启动服务器
    void start();

private:
    // 上报链接相关信息的回调函数
    void onConnection(const TcpConnectionPtr &);

    // 上报读写事件相关信息的回调函数
    void onMessage(const TcpConnectionPtr &,
                   Buffer *,
                   Timestamp);

    TcpServer _server; // 组合的muduo库，实现服务器功能的类对象
    EventLoop *_loop;  // 指向事件循环对象的指针
};

#endif
