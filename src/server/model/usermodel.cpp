// model层代码 操纵数据库

#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

// User表的增加用户信息方法
bool UserModel::insert(User &user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user(name,password,state) value('%s','%s','%s')",
            user.getName().c_str(), user.getPassword().c_str(), user.getState().c_str());
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户信息数据 生成的主键ID
            user.setID(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false; // 注册失败
}

// 根据用户号码查询用户信息
User UserModel::query(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select *from user where id=%d", id);

    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql); // 查询操作 在db.cpp中  //res是个指针，肯定要释放资源
        if (res != nullptr)
        {
            // 查询一行用户信息 ；mysql_fetch_row 查看他的引用是 MYSQL_ROW
            MYSQL_ROW row = mysql_fetch_row(res);

            if (row != nullptr)
            {
                User user;
                user.setID(atoi(row[0])); // atoi转成整数  row[]可直接加中括号，在RES的引用中可看到
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);

                mysql_free_result(res); // 释放资源  否则内存不断泄露
                return user;            // 返回user;
            }
        }
    }
    // 如果没有查询到该用户
    return User();
    // User(int id = -1, string name = "", string pwd = "", string state = "offline") -----User.hpp
    // 返回默认值，则证明查无此人
}

// 更新用户状态
bool UserModel::updateState(User user)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "update user set state ='%s' where id=%d", user.getState().c_str(), user.getID());
    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false; // 更新失败
}

// 重置用户的状态信息
void UserModel::resetState()
{
     // 1.组装sql语句
    char sql[1024] ="update user set state ='offline' where state='online'";

    // 2.连接数据库
    MySQL mysql;
    if (mysql.connect())
       mysql.update(sql);
       
}