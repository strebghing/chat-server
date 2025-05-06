#include "json.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//这两个头文件，一个是为了声明信号量进行线程间的通信，另一个是为了将两个线程共同操作的变量声明成原子变量
#include <semaphore.h>
#include <atomic>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前登陆系统的用户信息
User g_currentUser;
// 记录当前登录用户的好友列表,这样不用去服务器拉去信息，效率高一些
vector<User> g_currentUserFriendList;
// 记录当前登录用户的群组列表
vector<Group> g_currentUserGroupList;
// 控制聊天页面程序
bool isMainMenuRunning = false;
// 用于读写线程之间的通信，登录和注册操作不可能同时进行，所以只需一个信号量
sem_t rwsem;
// 记录登录状态，原子变量，因为两个线程都要操作，初始值为0
atomic_bool g_isLoginSuccess{false};

// 显示当前登录用户的基本信息
void showCurrentUserData();
// 接收线程
void readTaskHandle(int clinetfd);
// 获取系统时间（聊天需要传输时间信息）
string getCurrentTime();
// 主聊天页面程序
void mainMenu(int clientfd);

// 客户端实现，main是发送线程，子线程用作接收线程。(发送消息的同时，也能够接收到消息)
int main(int args, char **argv)
{
    // 命令行参数超过3个，命令无效
    if (args != 3)
    {
        cerr << "socket invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    // 解析通过命令行参数 传递的IP和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientfd == -1)
    {
        cerr << "socket create error" << endl;
        exit(-1);
    }

    // 填写client 需要连接的server信息 IP+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)) == -1)
    {
        cerr << "connect server error" << endl;
        close(clientfd);
        exit(-1);
    }

    // 初始化读写线程通信的信号量
    sem_init(&rwsem, 0, 0);

    // 连接服务器成功，开启子线程接收
    std::thread readTask(readTaskHandle, clientfd);
    readTask.detach(); // 回收线程

    // main用来接收用户输入，负责发送数据
    for (;;)
    {
        // 显示首页页面菜单
        cout << "===============================" << endl;
        cout << "1. login" << endl;
        cout << "2. register" << endl;
        cout << "3. quit" << endl;
        cout << "===============================" << endl;
        cout << "choice: ";
        int choice = 0;
        cin >> choice;
        cin.get(); // 读掉缓冲区残留的回车
        switch (choice)
        {
        case 1: // 登录业务
        {
            int id;
            char pwd[50] = {0};
            cout << "login userid: ";
            cin >> id;
            cin.get();
            cout << "login password: ";
            cin.getline(pwd, 50);

            // 按服务器端的json格式发送登录信息
            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            g_isLoginSuccess = false;

            // 发送数据
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                // 发送失败
                cerr << "send login message error: " << request << endl;
            }
            // 子线程接收 从没停止，再次登陆的时候，主线程也在receive，需要修改
            // 修改后的版本：子线程负责接收，主线程只负责发送，不再接收； 子线程在连接成功后，立马开启；
            // 主线程send之后，要等待子线程的信号量（告诉主线程，我接到你send的信息了，响应登录操作，失败还是成功）
            //  static int threadnum=0;
            //  if(threadnum==0){
            //      std::thread readTask(readTaskHandle, clientfd);
            //      readTask.detach(); // 回收线程
            //      threadnum++;
            //  }

            sem_wait(&rwsem); // 等待子线程唤醒，判断子线程是否正确接收到主线程信息，并成功响应

            if (g_isLoginSuccess)
            {
                // 登陆成功，进入聊天主菜单页面
                isMainMenuRunning = true;
                mainMenu(clientfd);
            }
        }
        break;
        case 2: // 注册业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username: （不超过50个字符）";
            cin.getline(name, 50); // 读取整行
            cout << "user password: （不超过50个字符）";
            cin.getline(pwd, 50);

            // 按服务器端解析的规则，发送需要服务端解析度json数据
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            // 发送数据
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                // 发送失败
                cerr << "send register message error: " << request << endl;
            }
            
            sem_wait(&rwsem);
        }
        break;

        case 3: // 退出业务
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr << "invalid input!!" << endl;
            break;
        }
    }
    return 0;
}

// 处理登录的响应逻辑
void doLoginResponae(json &response)
{
    if (response["errno"].get<int>() != 0)
    {
        // 登陆失败
        cerr << response["errmsg"] << endl;
        g_isLoginSuccess = false;
    }
    else // 登陆成功
    {
        // 服务端的登录业务成功，要记录当前登录用户的好友、群组等信息
        g_currentUser.setId(response["id"].get<int>());
        g_currentUser.setName(response["name"]);

        // 记录当前登录用户的好友信息
        if (response.contains("friends"))
        {
            // 初始化
            g_currentUserFriendList.clear();

            vector<string> vec = response["friends"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                User u;
                u.setId(js["id"].get<int>());
                u.setName(js["name"]);
                u.setState(js["state"]);
                g_currentUserFriendList.push_back(u);
            }
        }

        // 记录当前登录用户的群组信息
        if (response.contains("groups"))
        {
            //
            g_currentUserGroupList.clear();

            vector<string> vec1 = response["groups"];
            for (string &str1 : vec1)
            {
                json gjs = json::parse(str1);
                Group g;
                g.setId(gjs["id"].get<int>());
                g.setName(gjs["groupname"]);
                g.setDesc(gjs["groupdesc"]);

                vector<string> vec2 = gjs["users"];
                for (string &str2 : vec2)
                {
                    GroupUser gu;
                    json js = json::parse(str2);
                    gu.setId(js["id"].get<int>());
                    gu.setName(js["name"]);
                    gu.setState(js["state"]);
                    gu.setRole(js["role"]);
                    g.getUsers().push_back(gu);
                }

                g_currentUserGroupList.push_back(g);
            }
        }

        // 显示登录用户的基本信息
        showCurrentUserData();

        // 显示当前用户的离线消息
        if (response.contains("offlinemsg"))
        {
            vector<string> vec = response["offlinemsg"];
            for (string &str : vec)
            {
                json js = json::parse(str);
                int msgtype = js["msgid"].get<int>();
                // time + [id] + name + " said " + xxx
                if (ONE_CHAT_MSG == msgtype)
                {
                    cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
                else if (GROUP_CHAT_MSG == msgtype)
                {
                    cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                         << " said: " << js["msg"].get<string>() << endl;
                }
            }
        }

        g_isLoginSuccess = true;
    }
}

//处理注册的响应逻辑
void doRegResponse(json &response)
{
    if (response["errno"].get<int>() != 0)
    {
        // 判断响应信息中，是否注册成功
        cerr << "name is already exist,register error!" << endl;
    }
    else
    {
        cout<< "name register success, userid is " << response["id"] << ",do not forget it!" << endl;
    }
}

// 接收线程
void readTaskHandle(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0); // 阻塞了
        if (len == -1 || len == 0)
        {
            close(clientfd);
            exit(-1);
        }

        // 接收ChatServer转发的数据，反序列化生成json数据对象
        json js = json::parse(buffer);
        int msgtype = js["msgid"].get<int>();
        if (ONE_CHAT_MSG == msgtype)
        {
            cout << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (GROUP_CHAT_MSG == msgtype)
        {
            cout << "群消息[" << js["groupid"] << "]: " << js["time"].get<string>() << "[" << js["id"] << "]" << js["name"].get<string>()
                 << " said: " << js["msg"].get<string>() << endl;
            continue;
        }
        if (LOGIN_MSG_ACK == msgtype)
        {
            doLoginResponae(js);
            sem_post(&rwsem); // 通知主线程，登录结果处理完成
            continue;
        }
        if (REG_MSG_ACK == msgtype)
        {
            doRegResponse(js);
            sem_post(&rwsem); // 通知主线程，注册结果处理完成
            continue;
        }
    }
}

// 显示当前登录用户的基本信息
void showCurrentUserData()
{
    cout << "======================================= login user =======================================" << endl;
    cout << " current login user ==> id:" << g_currentUser.getId() << " name: " << g_currentUser.getName() << endl;
    cout << "-------------------------------------friend list -----------------------------------------" << endl;
    // 朋友列表不为空
    if (!g_currentUserFriendList.empty())
    {
        // 遍历输出朋友用户信息
        for (User u : g_currentUserFriendList)
        {
            cout << u.getId() << " " << u.getName() << " " << u.getState() << endl;
        }
    }
    cout << "------------------------------------group list -------------------------------------------" << endl;
    // 群组列表不为空
    if (!g_currentUserGroupList.empty())
    {
        // 遍历输出群组信息
        for (Group g : g_currentUserGroupList)
        {
            cout << g.getId() << " " << g.getName() << " " << g.getDesc() << endl;
            // 遍历输出群组里的成员用户信息
            for (GroupUser gu : g.getUsers())
            {
                cout << gu.getId() << " " << gu.getName() << " " << gu.getState() << " " << gu.getRole() << endl;
            }
        }
    }
    cout << "=======================================end===============================================" << endl;
}

// 获取系统时间
string getCurrentTime()
{
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    struct tm *ptm = localtime(&tt);
    char date[60] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            (int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
            (int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);

    return string(date);
}

// 系统支持的客户端命令列表
unordered_map<string, string> commanMap = {
    {"help", "显示所有支持的命令，格式help"},
    {"chat", "一对一聊天，格式chat:frinedid:message"},
    {"addFriend", "添加好友，格式addFriend:friendid"},
    {"createGroup", "创建群组，格式createGroup:groupname:groupdesc"},
    {"addGroup", "加入群组，格式addGroup:groupid"},
    {"groupChat", "群聊，格式groupChat:groupid:message"},
    {"loginout", "注销，格式loginout"},

};
// 声明命令的处理器
void help(int fd = 0, string str = ""); // 设置默认参数值，为了注册function时统一参数
void chat(int, string);
void addFriend(int, string);
void createGroup(int, string);
void addGroup(int, string);
void groupChat(int, string);
void loginout(int, string);
// 注册命令处理器
unordered_map<string, function<void(int, string)>> commanHandleMap = {
    {"help", help}, {"chat", chat}, {"addFriend", addFriend}, {"createGroup", createGroup}, {"addGroup", addGroup}, {"groupChat", groupChat}, {"loginout", loginout}};

// 主聊天页面程序
void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    while (isMainMenuRunning)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if (idx == -1)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commanHandleMap.find(command);
        if (it == commanHandleMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }
        // 调用相应命令的事件处理回调函数，mianMenu对修改封闭，添加新功能不需要修改mainMenu函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx));
    }
}

// help函数定义
void help(int, string)
{
    cout << "show command list: " << endl;
    for (auto &p : commanMap)
    {
        cout << p.first << " : " << p.second << endl;
    }
    cout << endl;
}

// caht 函数定义
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["to"] = friendid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send chat message error!" << endl;
    }
}

// 添加好友函数定义
void addFriend(int clientfd, string str)
{
    int frinedid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = frinedid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addFriend message error!" << endl;
    }
}

// 创建群组函数
void createGroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "createGroup command invalid!" << endl;
        return;
    }
    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send createGroup message error ->" << buffer << endl;
    }
}

// 添加群组函数
void addGroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send addGroup message error->" << buffer << endl;
    }
}

// 群聊函数
void groupChat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "groupChat command invalid!" << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx);
    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();
    string buffer = js.dump();
    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send groupChat message error ->" << buffer << endl;
    }
}

void loginout(int clientfd, string)
{
    json js;
    js["msgid"] = LOGINOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (len == -1)
    {
        cerr << "send loginut msg error->" << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}
