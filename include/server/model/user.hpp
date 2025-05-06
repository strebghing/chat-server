#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

//匹配数据库User表的ORM类（object relationship Map） 
//这个ORM类负责说明表的属性(映射表的字段)，操作数据表--->通过对应的操作类去做
class User{
public:
//有参构造：赋初值
    User(int id=-1,string name="",string pwd="",string state="offline"){
        this->id=id;
        this->name=name;
        this->password=pwd;
        this->state=state;
    }

    //成员变量的setter方法
    void setId(int id){this->id=id;}
    void setName(string name){this->name=name;}
    void setPassword(string pwd){this->password=pwd;}
    void setState(string state){this->state=state;}

    //成员变量的getter方法，因为成员变量私有，提供接口访问
    int getId(){return this->id;}
    string getName(){return this->name;}
    string getPassword(){return this->password;}
    string getState(){return this->state;}

protected:
    int id;
    string name;
    string password;
    string state;
};

#endif