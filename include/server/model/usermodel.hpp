//分离数据层和业务层的代码---ORM

#ifndef USERMODEL_H
#define USERMODEL_H

#include "user.hpp"
#include <string>
using namespace std;

// User表的数据操作类（提供方法，就是表的增删改查
class UserModel
{

public:
    // User表的增加方法
    bool insert(User &user);
    
    //根据用户号码查询用户信息
    User query(int id);   //也可以选用控制指针 判断是否查询到用户信息


    //更新用户状态
    bool updateState(User user);

    //重置用户的状态信息
    void resetState();

};

#endif