#include "offlinemessagemodel.hpp"
#include "db.h"

// 存储用户离线消息
void offlineMsgModel::insert(int userid, string msg)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into offlinemessage values(%d,'%s')", userid, msg.c_str());
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 删除用户的离线消息

void offlineMsgModel::remove(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "delete from  offlinemessage where userid =%d", userid);
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}

// 查询用户的离线消息
vector<string> offlineMsgModel::query(int userid)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select message from offlinemessage where userid=%d", userid);

    // 2.连接数据库
    vector<string> vec; 
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 查询操作 在db.cpp中  //res是个指针，肯定要释放资源
        if (res != nullptr)
        {
            // 查询一行用户信息 ；mysql_fetch_row 查看他的引用是 MYSQL_ROW
            //MYSQL_ROW row = mysql_fetch_row(res);

            //把userid用户 所有的离线消息 放到vector中返回
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr)    //可能存在多条消息
            { 
                vec.push_back(row[0]);                    //存放
            }

            mysql_free_result(res);
            return vec;
        }
    }
    return vec;
}