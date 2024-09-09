//分离数据层和业务层的代码---ORM

#ifndef GROUPMODEL_H
#define GROUPMODEL_H

#include "group.hpp"
#include <string>
#include <vector>
using namespace std;

// User表的数据操作类（提供方法，就是表的增删改查
//维护群组消息的操作接口方法
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    
    //加入群组
    User addGroup(int userid, int groupid, string role);   //加入小组的用户信息

    //查询用户所在群组信息
    vector<Group> queryGroup(int userid);                 //返回群组消息 查询群内好友列表（消息、状态、身份等等
    
    //用于群组聊天
    //根据指定的groupid 查询群组 用户id列表  
    //除了userid自己 主要用户群聊业务给其他成员群发消息
    vector<int> queryGroupUsers(int userid, int groupid);  //群消息     //因为之前已经/存储在线用户的通信链接 ——登录成功即为存储成功
    //unordered_map<int, TcpConnectionPtr> _userConnMap;   通过左侧map表拿到成员用户的id ,通过服务器转发消息

};

#endif