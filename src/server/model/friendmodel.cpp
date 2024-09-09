#include "friendmodel.hpp"
#include "db.h"

// 添加好友关系
void FriendModel::insert(int userid, int friendid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into friend  values(%d,%d)", userid, friendid);
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 返回用户好友列表 friendid （User表 id state name）
// 两个表的联合查询即可
vector <User> FriendModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.name,a.state from user a inner join friend b on b.friendid=a.id where b.userid=%d", userid);

    // 2.连接数据库
    vector<User> vec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 查询操作 在db.cpp中  //res是个指针，肯定要释放资源
        if (res != nullptr)
        {
            // 查询一行用户信息 ；mysql_fetch_row 查看他的引用是 MYSQL_ROW
            // MYSQL_ROW row = mysql_fetch_row(res);

            // 把userid用户 所有的离线消息 放到vector中返回
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr) // 可能存在多条消息
            {
                User user;
                user.setID(atoi(row[0]));
                user.setName((row[1]));
                user.setState((row[2]));
                vec.push_back(user);
            }

            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}