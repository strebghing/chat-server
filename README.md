# chat-server
可以工作在nginx tcp负载均衡环境中的集群聊天服务器和命令行显示的客户端源码 基于moduo实现
## 项目使用
拉取到本地后，进入build文件夹，运行`cmake`和`make`命令，编译项目；编译完成后，进入到bin目录下，先运行服务器端代码`./ChatServer ip port` ，再运行客户端代码Client
`./ChatClient ip port`。服务器的port与nginx配置的负载均衡的server有关，客户端的port与nginx配置的负载均衡的监听端口有关
## 项目文件夹说明
- bin文件夹：存放可执行程序，一个是客户端ChatClient，一个是服务器端ChatServer
- build文件夹：存放的是编译后的内容
- include文件夹：分为server和client所用的头文件，以及双方都会用到的头文件public
- src文件夹：存放的是头文件对应的cpp源文件
- thirdparty文件夹：存放用到的第三方库，这里用了json.hpp
## 项目模块组成
- 网络模块是通过muduo库来实现的，在网络模块中存储了与网络请求一一对应的handler处理，实现与业务模块的分离
- 业务模块中注册各种业务函数的回调，通过定义数据操作类对象访问数据库，与数据库模块分离
- 数据库模块通过定义类，实现类方法进行对后端MySQL数据库的访问
- 使用redis的发布-订阅功能，作为消息队列，实现跨服务器的通信
- 客户端的实现集中在src/client/main.cpp，使用主线程进行消息的发送，利用readTask注册子线程专门进行消息的接收，两线程间需要通信
