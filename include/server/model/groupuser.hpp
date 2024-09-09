#ifndef GROUPUSER_H
#define GROUPUSER_H

#include "user.hpp"


// 群组用户 多了一个role角色信息  从User类直接继承 复用User的其他信息
class GroupUser :public User
{

public:

    void setRole(string role) { this->role = role; }
    string getRole() { return this->role; }

private:
    string role;    //管理员以及群主成员变量
};

#endif

//继承User基本信息 比如id name 等等 
//又添加了一个特殊身份 role
