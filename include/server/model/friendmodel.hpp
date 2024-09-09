#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include <string>
#include <vector>
using namespace std;

// 维护好友的操作接口方法   
//简化版本 无删除好友 没有好友同意添加等等
class FriendModel
{

public:
    // 添加好友关系
    void insert(int userid, int friendid);

    // 返回用户好友列表 friendid （User表 id state name）
    //两个表的联合查询即可
    vector <User> query(int userid);

};

#endif
