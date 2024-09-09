#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

// 匹配User表的ORM类（映射对象类
class User
{


public:
    User(int id = -1, string name = "", string pwd = "", string state = "offline")
    {
        this->id = id;
        this->name = name;
        this->password = pwd;
        this->state = state;
    }
    void setID(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setPassword(string pwd) { this->password = pwd; }
    void setState(string state) { this->state = state; }

    int getID() { return this->id; }
    string getName() { return this->name; }
    string getPassword() { return this->password; }
    string getState() { return this->state; }

private:
    int id;
    string name;
    string password;
    string state;
};

#endif
