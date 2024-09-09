#include "groupmodel.hpp"
#include "db.h"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname,groupdesc) value('%s','%s')",
            group.getName().c_str(), group.getDesc().c_str());
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户信息数据 生成的主键ID
            group.setID(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false; // 注册失败
}

// 加入群组
User GroupModel::addGroup(int userid, int groupid, string role) // 加入小组的用户信息
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser values(%d,%d,'%s')",
            groupid, userid, role.c_str());
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户所在群组信息
vector<Group> GroupModel::queryGroup(int userid) // 返回群组消息 查询群内好友列表（消息、状态、身份等等
{
    /*
    1. 先根据 userid 在group 表中查询出该用户 所属的群组消息
    2. 再根据群组消息 查询属于该群组所有的用户基本信息
    */

    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from allgroup a inner join groupuser b on a.id=b.groupid where b.userid=%d", userid);

    // 2.连接数据库
    vector<Group> groupVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {

            MYSQL_ROW row;

            // 查询userid所有的群组消息
            while ((row = mysql_fetch_row(res)) != nullptr) // 可能存在多条消息
            {
                Group group;
                group.setID(atoi(row[0]));
                group.setName((row[1]));
                group.setDesc((row[2]));
                groupVec.push_back(group);
            }

            mysql_free_result(res);
        }
    }

    // 查询群内的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from user a inner join \
    groupuser b on b.userid=a.id where b.groupid=%d",
                group.getID()); // User表和Group表的联合查询

        MYSQL_RES *res = mysql.query(sql); // 查询操作 在db.cpp中  //res是个指针，肯定要释放资源
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 可能存在多条消息
            {
                GroupUser user;
                user.setID(atoi(row[0]));
                user.setName((row[1]));
                user.setState((row[2]));
                user.setRole((row[3]));
                group.getUsers().push_back(user);
            }

            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 用于群组聊天
// 根据指定的groupid 查询群组 用户id列表
// 除了userid自己 主要用户群聊业务给其他成员群发消息
// vector<int> queryGroupUsers(int userid, int groupid);  //群消息     //因为之前已经/存储在线用户的通信链接 ——登录成功即为存储成功
// unordered_map<int, TcpConnectionPtr> _userConnMap;   通过左侧map表拿到成员用户的id ,通过服务器转发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid=%d and userid!=%d",groupid, userid);  //我发群聊消息 不需要转发给我自己 只需要群内其他用户知晓

    // 2.连接数据库
    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 可能存在多条消息
            {
                idVec.push_back(atoi(row[0]));
            }

            mysql_free_result(res);
        }
    }
    return idVec;
}