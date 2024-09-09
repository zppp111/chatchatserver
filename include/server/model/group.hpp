#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;

// 匹配User表的ORM类（映射对象类  -----照猫画老虎
class Group
{

public:
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }
    void setID(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }

    int getID() { return this->id; }
    string getName() { return this->name; }
    string getDesc() { return this->desc; }
    vector<GroupUser> &getUsers(){return this->users;}   
    //扩充组，然后把成员放进vector中 给业务层去使用
    //mysql可以多表查询  避免多次连接多次是释放 浪费资源  也可以提升效率

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> users;       //4个成员变量
};

#endif
