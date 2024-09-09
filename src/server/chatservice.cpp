/*
网络模块和业务模块耦合的核心代码
*/

#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>
#include <string>
#include <vector>

using namespace muduo;
using namespace std;
using namespace boost;

// 获取单例对象的接口函数 static
Chatservice *Chatservice::instance()
{
    static Chatservice service;
    return &service;
}

// 注册消息以及对应的handler回调操作 oop
//  hpp中构造函数私有化
Chatservice::Chatservice()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&Chatservice::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&Chatservice::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&Chatservice::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&Chatservice::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, std::bind(&Chatservice::addFriend, this, _1, _2, _3)});

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&Chatservice::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&Chatservice::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&Chatservice::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    if (_redis.connect())
    {
        // 设置上报消息的回调
        _redis.init_notify_handler(std::bind(&Chatservice::handleRedisSubscribeMessage, this, _1, _2));
       // _pool.start(1);
       // _pool.run(boost::bind(&Redis::observer_channel_message,&_redis));
    }
}

// 处理服务器异常操作 业务重置
void Chatservice::reset()
{
    // 把所有用户的状态改成offline
    _userModel.resetState();
}

// 获取消息对应的处理器
MsgHandler Chatservice::getHandler(int msgid)
{
    // 记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        // LOG_ERROR << "msgid:" << msgid << "can't find handler!";
        // string errstr="msgid:" +msgid + "can't find handler!";
        // 返回一个默认的处理器 空操作
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid:" << msgid << "can't find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// 业务数据分开方法：ORM框架 object relation map 对象关系映射
// 业务层操作的都是对象 DAO层还是具体的数据对象

// 处理登录业务
void Chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{

    // LOG_INFO << "do login service!!!"; 暂时注释掉 开始认真写注册业务 业务操作都是数据对象 无字段
    int id = js["id"].get<int>(); // 转成整形
    string password = js["password"];

    // 传入一个id值 返回id主键对应的数据（从数据库中
    User user = _userModel.query(id);
    if (user.getID() == id && user.getPassword() == password)
    { // id不等于-1（默认值）

        if (user.getState() == "online")
        {
            // 用户已经登陆 不允许重复登陆
            json response;
            response["msgid"] = LOGIN_MSG_ACK;                       // 响应消息
            response["errno"] = 2;                                   // 响应消息是2，登录失败
            response["errmsg"] = "该账号已经登录，请重新输入新账号"; // 响应消息是
            conn->send(response.dump());
        }
        else
        {
            // 登陆成功                     //=》记录用户的链接信息
            {

                lock_guard<mutex> lock(_connMutex); // 引用接收 ，非指针 可看lock guard定义
                _userConnMap.insert({id, conn});
            }
            // 大括号一括 只保证这个作用域  锁住锁住

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            // 登陆成功                    //-》更新用户状态信息offlin 变成online
            user.setState("online");
            _userModel.updateState(user); // 刷新一下

            json response;
            response["msgid"] = LOGIN_MSG_ACK; // 响应消息
            response["errno"] = 0;             // 响应消息是0，响应成功
            response["id"] = user.getID();     // 携带用户的id 呈现出来
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec; // 读取离线消息
                // 读取该用户的离线消息后 把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            //  查询该用户的好友信息
            vector<User> userVec = _friendModel.query(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (User &user : userVec)
                {
                    json js;
                    js["id"] = user.getID();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            //  查询该用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroup(id);
            if (!groupuserVec.empty())
            {
                // group:[{groupid:[xxx xxx xxx xxx]}]
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getID();
                    grpjson["groupname"] = group.getName();
                    grpjson["groupdesc"] = group.getDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getUsers())
                    {
                        json js;
                        js["id"] = user.getID();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 1.该用户不存在2.用户存在密码或者用户名错误登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;         // 响应消息
        response["errno"] = 1;                     // 响应消息是1，登录失败
        response["errmsg"] = "用户名或者密码错误"; // 响应消息是
        conn->send(response.dump());
    }
}

// 处理注册业务
void Chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // LOG_INFO<<"do reg service!!!";   暂时注销 开始认真写注册业务 业务操作都是数据对象 无字段
    // 用户的name password
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPassword(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功！
        json response;
        response["msgid"] = REG_MSG_ACK; // 响应消息
        response["errno"] = 0;           // 响应消息是0，响应成功
        response["id"] = user.getID();   // 携带用户的id 呈现出来

        // 以上就组装好了json 通过网络发送给客户端
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK; // 响应消息
        response["errno"] = 1;           // 响应消息是1，响应失败
        response["errmsg"] = 1;          // 响应消息是

        // 以上就组装好了json 通过网络发送给客户端
        conn->send(response.dump());
    }
}

// 一对一聊天业务
void Chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>(); // 获得接收方to的id

    {
        lock_guard<mutex> lock(_connMutex); // 线程安全 --围绕map表
        auto it = _userConnMap.find(toid);  // 从表中扒他的id
        if (it != _userConnMap.end())
        {
            // toid在线 转发消息
            it->second->send(js.dump()); // 消息中转 服务器主动推送消息给toid用户
            return;
        }
    }
    // 查询toid是否在线
    User user = _userModel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        
        return;
    }

    // toid不在线 存储离线消息
    _offlineMsgModel.insert(toid, js.dump());
}

// 添加好友业务(msgid friendid id)
void Chatservice::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    // 存储好友信息
    _friendModel.insert(userid, friendid);
}

// 创建群组业务
void Chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    { // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getID(), "creator");
    }
}

// 加入群组业务
void Chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    // 存储群组好友信息
    _groupModel.addGroup(userid, groupid, "normal");
}

// 群组聊天业务
void Chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);
    lock_guard<mutex> lock(_connMutex); // 线程安全 --围绕map表

    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发消息
            it->second->send(js.dump()); // 消息中转 服务器主动推送消息给id用户
        }
        else
        {
            User user = _userModel.query(id);
            if (user.getState() == "online")
            {
                _redis.publish(id, js.dump());
                return;
            }
            else // id不在线 存储离线消息
            {
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 从redis消息队列中获取订阅的消息
void Chatservice::handleRedisSubscribeMessage(int userid, string msg)
{
    json js=json::parse(msg.c_str());
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        it->second->send(js.dump());
        return;
    }

    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}

// 处理客户端异常退出
void Chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    { // 有的用户可能正在登录 有的异常 注意线程安全
        lock_guard<mutex> lock(_connMutex);

        // 遍历哈希表
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn) // 比较智能指针TcpConnectionPtr &conn
            {
                // 从map表删除用户的连接信息
                user.setID(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 用户注销下线 向redis取消订阅channel(=id)
    _redis.unsubscribe(user.getID()); 

    // 更新用户的登陆状态 offline
    if (user.getID() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 处理客户端注销
void Chatservice::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    // 用户注销，相当于就是下线，在redis中取消订阅通道
    _redis.unsubscribe(userid);

    // 更新用户的登陆状态 offline
    User user(userid, "", "", "offline");
    _userModel.updateState(user); // updateState去看他的声明定义F12 又转到User类看他的定义
}
