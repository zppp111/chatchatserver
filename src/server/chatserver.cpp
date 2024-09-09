#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"
#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

///////// 初始化服务器对象////////////
ChatServer::ChatServer(EventLoop *loop,
                       const InetAddress &listenAddr,
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
    // 一个主reactor 主线程——新用户链接  三个subreactor 工作线程——新用户的读写事件的处理
}

///////// 启动服务器////////////
void ChatServer::start()
{
    _server.start();
}

///////// 链接监听onConnection和读写事件监听onMessage////////////

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 客户端断开连接
    if (!conn->connected()) // 连接失败 释放信号
    {
        Chatservice::instance()->clientCloseException(conn); // 客户端异常关闭
        conn->shutdown();                                    // 下线
    }
}

// 上报读写事件相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    string buf = buffer->retrieveAllAsString(); // 从数据缓冲区拿出来
    // 测试，添加json打印代码
    cout << buf << endl;

    json js = json::parse(buf); // 数据反序列化
    // 目的：完全解耦网络模块的代码和业务模块代码
    // 通过js["msgid"] ,事先绑定回调操作,一个id对应一条操作   来获取-》业务处理器handler-》conn js对象 time
    auto msgHandler = Chatservice::instance()->getHandler(js["msgid"].get<int>()); // json中的数据转成实际整型
    // 回调消息绑定好的事件处理器 来执行相应的业务处理
    msgHandler(conn, js, time);
}
