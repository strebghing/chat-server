#include "json.hpp"
using json=nlohmann::json;

#include <iostream>
#include<vector>
#include<map>
#include<string>
using namespace std;

//json序列化1
void func1(){
    json js;
    js["msg_type"]=2;
    js["from"]="zhang san";
    js["to"]="li si";
    js["msg"]="hello, we are connecting";

    string sendBuf=js.dump();//json转成字符串类型
    cout<<sendBuf.c_str()<<endl;
}

void func2(){
    json js;
    js["id"]={1,2,3,4,5};
    js["name"]="zhang san";
    js["msg"]["zhang san"]="hello, zhang san";
    js["msg"]["li si"]="hello, li si";

    js["msg"]={{"zhang san","hello, zhang san"},{"li si","hello, li si"}};
    cout<<js<<endl;
}

int main(){
    func1();
    return 0;
}
