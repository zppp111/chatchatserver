#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <functional>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h> //信号量
#include <atomic>      //C+11

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

//都是在子线程里进行的 可以用vector 不涉及到线程安全问题
// 记录当前系统登陆的用户信息
User g_currentUser;

// 记录当前登录用户的好友信息
vector<User> g_currentUserFriendList;

// 记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

// 控制主菜单页面程序
bool isMainMenuRuning = false;

// 用于读写线程之间的通信
sem_t rwsem;
// 记录登录状态
atomic_bool g_isLoginSuccess{false};

// 接收线程
void readTaskHandler(int clientfd);

// 主菜单页面程序
void mainMenu(int clientfd); // 登录成功才能进这个
// 显示当前登陆成功用户的基本信息
void showCurrentUserData();
// 获取系统时间（聊天信息需要增加时间  c++11
string getCurrentTime();

// 说的非常不详细 一些代码可能得重新看 重新查
//  聊天客户端程序实现 main线程用作发送线程 子线程用作接收线程
int main(int argc, char **argv)
{
    if (argc < 3) // 命令个数
    {
        cerr << "command invaild! example:./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }

    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client需要连接的server信息的ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server的连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1); // 释放资源 退出
    }

    // 初始化读写线程通信用的信号量
    sem_init(&rwsem, 0, 0);


    // 连接服务器成功，启动子线程，接收消息
    std::thread readTask(readTaskHandler, clientfd); //// pthread_create
    readTask.detach();                               //// pthread_detach

    // main线程用于接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页面菜单 登录 注册 退出
        cout << "======================-" << endl;
        cout << "1. Login" << endl;
        cout << "2. Register" << endl;
        cout << "3. Quit" << endl;
        cout << "======================-" << endl;
        cout << "Choice:";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区域残留的回车   输入的是整数+回车

        switch (choice)
        {
        case 1: // login 登录业务 （id+password）
        {
            int id = 0;
            char password[50] = {0};
            cout << "Userid:";
            cin >> id;
            cin.get();
            cout << "Userpassword:";
            cin.getline(password, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = password;
            string request = js.dump(); // 序列化成字符串

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == 1)
            {
                cerr << "send login msg error:" << request << endl;
            }

            sem_wait(&rwsem); // 等待信号量，由子线程处理完登录的响应消息后，通知这里

            if (g_isLoginSuccess)
            { // 进入聊天页面
                isMainMenuRuning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // register业务
        {
            char name[50] = {0};
            char password[50] = {0};
            cout << "Username:";
            cin.getline(name, 50);
            cout << "Userpassword:";
            cin.getline(password, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = password;
            string request = js.dump(); // 序列化成字符串

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == 1)
            {
                cerr << "send register msg error:" << request << endl;
            }
            sem_wait(&rwsem); // 等待信号量，由子线程处理完注册的响应消息后，通知这里
        }
        break;
        case 3: // quit业务
            close(clientfd);
            sem_destroy(&rwsem); //释放信号量  （对应初始化）
            exit(0);
        default:
            cerr << "invaild input!" << endl;
            break;
        }
    }
    return 0;
}

// 处理登录的响应逻辑
void doLoginResponse(json &responsejs)
{
    if (0 != responsejs["errno"].get<int>()) // 登录失败
    {
        cerr << responsejs["errmsg"] << endl;
        g_isLoginSuccess=false;  //初始设置false,登陆成功置于true。
    }
    else // 登陆成功
    {
        // 记录当前用户的id和name
        g_currentUser.setID(responsejs["id"].get<int>());
        g_currentUser.setName(responsejs["name"]);

        // 记录当前用户的好友列表信息
        if (responsejs.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = responsejs["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User user;
                user.setID(js["id"].get<int>());
                user.setName(js["name"]);
                user.setState(js["state"]);
                g_currentUserFriendList.push_back(user); // 反序列化 填入全局 vector 中g_currentUserFriendList
            }
        }

        // 记录用户的群组列表信息
        if (responsejs.contains("groups"))
        {
            // 初始化操作
            g_currentUserGroupList.clear();

            vector<string> vec1 = responsejs["groups"];
            for (string &groupstr : vec1)
            {
                json groupjs = json::parse(groupstr);
                Group group;
                group.setID(groupjs["id"].get<int>());
                group.setName(groupjs["groupname"]);
                group.setDesc(groupjs["groupdesc"]);

                vector<string> vec2 = groupjs["users"];
                for (string &userstr : vec2)
                {
                    GroupUser user;
                    json js = json::parse(userstr);
                    user.setID(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }

                g_currentUserGroupList.push_back(group); // 反序列化 填入全局 vector 中g_currentUserFriendList
            }
        }

        // 显示登录用户基本信息 11.08 24.26
        showCurrentUserData();
        // 显示当前用户的离线消息 个人聊天消息或者群组消息
        if (responsejs.contains("offlinemsg"))
        {
            vector<string> vec = responsejs["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                // time+id+name+ said +xxx
                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                {
                    cout << "好友消息==> :" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                         << "  said: " << js["msg"].get<string>() << endl;
                }
                else
                {
                    cout << "群消息==>[ " << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                         << "  said: " << js["msg"].get<string>() << endl;
                }
            }
        }
        g_isLoginSuccess = true;
    }
}

// 处理注册的响应逻辑
void doRegResponse(json &responsejs)
{

    if (0 != responsejs["errno"].get<int>()) // 注册失败
    {
        cerr << "name is already exist ,register error!" << endl;
    }
    else // 注册成功
    {
        cout << "name  register success, userid is " << responsejs["id"]
             << ", do not forget it!" << endl;
    }
}

// 接收线程
void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 用户登出之后  直接阻塞 也响应不了   //bt打印显示阻塞 堆栈
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据  反序列化生成json数据对象
        json js = json ::parse(buffer); // 反序列化回网络的buffer 得到一个json对象
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << "好友消息==> :" << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                 << "  said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息==>[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"] << "] " << js["name"].get<string>()
                 << "  said: " << js["msg"].get<string>() << endl;
            continue;
        }

        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponse(js); // 处理登录响应的业务逻辑
            sem_post(&rwsem);    // 通知主线程，登录结果处理完成
            continue;
        }

        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 显示当前登陆成功用户的基本信息
void showCurrentUserData()
{
    cout << "======================LOGIN USER======================" << endl;
    cout << "current login user ==》 ID: " << g_currentUser.getID() << " Name: " << g_currentUser.getName() << endl;

    cout << "======================FRIEND LIST======================" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getID() << " " << user.getName() << " " << user.getState() << endl;
        }
    }

    cout << "======================GROUP LIST======================" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << " GroupID[" << group.getID() << "]:"
                 << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &user : group.getUsers())
            {
                cout << " UserID[" << user.getID() << "]:"
                     << " " << user.getName() << " " << user.getState() << " ["
                     << user.getRole() << "] " << endl;
            }
        }
    }
    cout << "======================================================" << endl;
}

//"help" command handler
void help(int fd = 0, string = "");
void chat(int, string);
void addfriend(int, string);
void creategroup(int, string);
void addgroup(int, string);
void groupchat(int, string);
//"quit" command handler
void loginout(int, string);

// 系统支持的客户端命令列表
unordered_map<string, string> commandMap = {
    {"help", "显示所有支持命令, 格式help"},
    {"chat", "一对一聊天, 格式chat:friendid:message"},
    {"addfriend", "添加好友, 格式addfriend:friendid"},
    {"creategroup", "创建群组, 格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组， 格式addgroup:groupid"},
    {"groupchat", "群聊, 格式groupchat:groupid:message"},
    {"loginout", "注销, 格式loginout"}};

// 注册系统支持的客户端命令
unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isMainMenuRuning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command; // 存储命令
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }

        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << " Invaild input command! " << endl;
            continue;
        }

        // 调用相应的命令的事件处理回调  mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx)); // 调用命令的方法
    }
}

//"help" command handler
void help(int, string)
{
    cout << "Show Command List >>>>>  " << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << ":" << p.second << endl;
    }
    cout << endl;
}

//"addfriend" command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getID();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send addfriend message error-> " << buffer << endl;
    }
}

//"chat" command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << " Chat command invaild! " << endl;
        return;
    }

    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getID();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send chat message error-> " << buffer << endl;
    }
}

//"creategroup" command handler
void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << " Creategroup command invaild! " << endl;
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send Creategroup message error-> " << buffer << endl;
    }
}
//"addgroup " command handler
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getID();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send aaddgroup message error-> " << buffer << endl;
    }
}

//"groupchat" command handler
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << " Groupchat command invaild! " << endl;
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getID();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send Groupchat message error-> " << buffer << endl;
    }
}

//"loginout" command handler
void loginout(int clientfd, string str) // 把消息发给服务端 服务端和客户端名称必须一一对应 保证他们使用的是相同的语言 否则会报错
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getID();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << " Send loginout message error-> " << buffer << endl;
    }
    else
    {
        isMainMenuRuning = false;
    }
}

// 获取系统时间 添加时间信息
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt); // 将time_t类型的时间转换为tm结构体形式
    // 格式化时间字符串
    char date[80] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    // 返回格式化后的时间字符串
    return std::string(date);
}