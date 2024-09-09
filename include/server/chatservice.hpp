// 服务层代码
#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>

#include <mutex> //锁

#include "redis.hpp"
#include "json.hpp"
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "usermodel.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>; // 消息id绑定的事件

// 聊天服务器业务类   给masg id映射一个事件回调
class Chatservice
{
public:
    // 获取单例对象的接口函数 static
    static Chatservice *instance(); // 暴露一个指针
    // 业务方法 处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理客户端注销
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);


    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 处理服务器异常操作 业务重置
    void reset();

    // 从redis消息队列中获取订阅的消息
    void handleRedisSubscribeMessage(int, string);

private:
    Chatservice(); // 构造函数私有化
    
    // 存储消息Id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap; // 消息处理器的表  消息id对应的处理操作

    // 存储在线用户的通信链接 ——登录成功即为存储成功
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    // 定义互斥锁，保证_userConnMap的线程安全
    mutex _connMutex;

    // 数操作类对象
    UserModel _userModel;

    // 离线消息类对象
    offlineMsgModel _offlineMsgModel;

    // 添加好友类对象
    FriendModel _friendModel;

    // 群聊业务
    GroupModel _groupModel;

    // redis操作对象
    Redis _redis;
};

#endif