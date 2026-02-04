/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: liliu <liliu@student.42.fr>                +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/26 16:58:36 by liliu             #+#    #+#             */
/*   Updated: 2026/02/04 18:51:35 by liliu            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include "../include/server.hpp"
#include "../include/client.hpp"
#include "../include/commands.hpp"
#include "../include/channel.hpp"
#include "../include/colors.hpp"
#include <sstream>
#include <vector>
#include <algorithm>
#include <cerrno>

Server::Server(int port, const std::string& password) : _port(port), _password(password), _serverName("ircserv")
{}

Server::~Server()
{
    //删除所有客户端
    for (size_t i = 0; i < listClients.size(); ++i)
    {
        delete listClients[i];
    }
    listClients.clear();
    //删除所有频道
    for (std::map<std::string, Channel*>::iterator it = listChannels.begin(); it != listChannels.end(); ++it)
    {
        delete it->second;
    }
    listChannels.clear();

    pollfds.clear();
    //清空 poll fd（pollfds 是服务器的“雷达”）
    if (_socketfd >= 0)
        close(_socketfd);
}
//服务器启动 start()
void Server::start(){
    //地址族（Address Family）这里意思是：我要用 IPv4 网络。
    /*
    AF_INET → IPv4（最常见）
    AF_INET6 → IPv6
    AF_UNIX → 本地进程通信
    SOCK_STREAM-》 面向连接，可靠（TCP）-〉我要一个 TCP socket
    SOCK_DGRAM-》无连接，不保证可靠（UDP）
    */
   //协议号通常填 0你帮我根据前面两个参数，自动选合适的协议”
   //创建一个基于 IPv4 的 TCP 网络通信接口，并返回一个 socket 编号给 _socketfd
    _socketfd = socket(AF_INET, SOCK_STREAM, 0);//服务器监听 socket
    if(_socketfd < 0)
        throw std::runtime_error("socket error");
    
    int opt = 1;//2️⃣ 设置端口可复用
    //设置 socket 的“属性/开关”
    //(设置哪个 socket, 设置层级（level）, 地址/端口复用, <value>1打开 SO_REUSEADDR, 0→ 关闭, 我传进去的数据有多大”)
    //地址/端口复用即使端口处在 TIME_WAIT 状态，也允许重新 bind; ex: bind: Address already in use
    //给这个 socket 打开“端口复用”开关，避免端口被占用导致无法重新绑定
    if (setsockopt(_socketfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        throw std::runtime_error("setsockopt error");

        //非阻塞 = 服务器不会因为一个客户端卡住
        //F_SETFL=设置文件状态标志（File Status Flags）
        //O_NONBLOCK=非阻塞标志
        //errno = EAGAIN 或 EWOULDBLOCK
        //阻塞（默认）没数据？那我就 等着。accept() → 没人连就卡住,recv() → 没数据就卡住
    if (fcntl(_socketfd, F_SETFL, O_NONBLOCK) < 0)//设置非阻塞模式
        throw std::runtime_error("fcntl error");
    sockaddr_in sockad;
    /*
    //sockaddr_in 是 IPv4 专用结构体
    struct sockaddr_in {
    sa_family_t sin_family; //AF_INET
    in_port_t sin_port; //  端口号（网络字节序）
    struct in_addr sin_addr; //IP 地址
    };
    */
    memset(&sockad, 0, sizeof(sockad));//地址结构体的大小
    sockad.sin_family = AF_INET;
    sockad.sin_port = htons(_port);//（port比如8080）
    sockad.sin_addr.s_addr = INADDR_ANY;
    //bind 绑定端口=把这个 socket 绑定到“本机的某个 IP + 端口”上。
    /*
    在你 socket() 之后：
    socket 只是个“空壳”
    还没指定在哪个 IP、哪个端口上等连接
    bind() 就是：
    告诉操作系统：这个 socket 负责监听这里。
    */
   //bind(_socketfd, (sockaddr*)&sockad, sizeof(sockad));
    //把 _socketfd 这个 socket 绑定到 sockad 指定的 IP 和端口上
    //bind() = 占住一个 IP + 端口，准备接客
    if(bind(_socketfd, (const sockaddr*)&sockad, sizeof(sockad)) < 0)//成功 → 返回 0失败 → 返回 -1
        throw std::runtime_error("bind error");
    //listen 监听连接， 允许最多 5 个“还没被 accept() 的连接”在门口排队(backlog（等待队列长度）)
    //listen()告诉操作系统：这是个服务器 socket，可以接收客户端连接了
    if(listen(_socketfd, 5) < 0)
        throw std::runtime_error("listen error");
    struct pollfd pfd;
    /*
    struct pollfd {
        int fd;// 文件描述符
        short events; // 关心的事件
        short revents;// 实际发生的事件（内核填）
    };
    */
   //把服务器 socket 加入 poll
    pfd.fd = _socketfd;//把 监听 socket（listen fd） 放进去, 不是客户端连接 fd
    pfd.events = POLLIN;//关心“可读事件”
    pfd.revents = 0;//初始化, 真正的值由 poll() 返回时填写。
    pollfds.push_back(pfd);//把监听 socket 加入 poll 监控集合
    loop();
    /*
    void loop() {
    while (true) {
        int n = poll(pollfds.data(), pollfds.size(), -1);

        for (auto &pfd : pollfds) {
            if (pfd.revents & POLLIN) {
                if (pfd.fd == _socketfd) {
                    // 新连接
                    accept();
                } else {
                    // 客户端发数据
                    recv();
                }
            }
        }
    }
}
*/
}

void Server::acceptclients(){
    /*
    struct sockaddr_in {
    sa_family_t sin_family; //AF_INET
    in_port_t sin_port; //  端口号（网络字节序）
    struct in_addr sin_addr; //IP 地址
    };
    */
    sockaddr_in clientaddr;
    socklen_t lenclient = sizeof(clientaddr);

    //打开门，把小朋友接进来, 如果没人：就等, 如果失败：就报错
    int clientfd = accept(_socketfd, (sockaddr*)&clientaddr, &lenclient);
    if (clientfd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return; // 正常情况，不是错误
        throw std::runtime_error("accept error");
    }

    //设置客户端 fd 为非阻塞, =给小朋友一个“不排队”的手环
    if (fcntl(clientfd, F_SETFL, O_NONBLOCK) < 0)
        throw std::runtime_error("fcntl error");
    //创建 Client 对象, 每个客户端 = 一个 Client
    /*
    ntohs(clientaddr.sin_port) = 把网络里的端口号，变成我们电脑能看懂的数字
    网络里端口号是 网络字节序（大端）, 电脑用的是 主机字节序
    inet_ntoa(clientaddr.sin_addr) = 把网络里的 IP 地址，变成我们电脑能看懂的点分十进制字符串
    sin_addr 是一个 32 位整数, 总结=把 IP 变成字符串
    */
    Client *C = new Client (clientfd, ntohs(clientaddr.sin_port), inet_ntoa(clientaddr.sin_addr));
    pollfd c;
    c.fd = C->getfd();
    c.events = POLLIN;
    c.revents = 0;
    //pollfds 就是你交给 poll() 的“监控名单”
    pollfds.push_back(c);//把客户端加入 poll
    /*
    POLLIN = 现在读，不会卡
    { if fd = listen fd : POLLIN 表示有新连接}
    { if fd = clients fd : POLLIN 表示 有新数据}
    { 同一个 POLLIN，不同人，不同意思}
    pollfds 是 poll 的监控名单
    poll 做三件事：
    看 fd
    看你关心啥（events）
    发生啥了，写在 revents
    */
   //listClients: 管理客户端对象（他是谁）
    listClients.push_back(C);//保存客户端信息（fd、ip、port、状态）
    std::cout << GREEN << "New client connected: " << C->getip() << ":" << C->getport() << RESET << std::endl << std::endl;
}

/*
poll 阻塞等待事件
  ↓
listen fd 有事件 → accept
  ↓
client fd 有事件 → recv
*/
void Server::loop(){
    while (true){
        //等待任何 fd 有事件, -1 = 永远等
        if(poll(pollfds.data(), pollfds.size(), -1) == - 1)
            throw std::runtime_error("poll error");
        //检查是否有新客户端
        if(pollfds[0].revents & POLLIN)//pollfds[0] 是 listen fd, 默认约定pollfds[0] = 监听 socket
            acceptclients();
        //处理客户端数据
        //pollfds[0] = 服务器
        //pollfds[1..] = 客户端
        for (size_t i = 1; i < pollfds.size(); ++i)//遍历所有客户端 fd
        {
            if(pollfds[i].revents & POLLIN)//有客户端发数据来了
            {
                handle_client_data(pollfds[i].fd);//处理客户端数据
            }
        }
    }
}

void Server::disconnectClient(int client_fd)
{
    std::cout << RED << "Client " << client_fd << " has disconnected." << RESET << std::endl << std::endl;

    for (size_t i = 1; i < pollfds.size(); ++i)
    {
        if (pollfds[i].fd == client_fd)
        {
            pollfds.erase(pollfds.begin() + i);
            break;
        }
    }

    for (size_t i = 0; i < listClients.size(); ++i)
    {
        if (listClients[i]->getfd() == client_fd)
        {
            std::map<std::string, Channel*>::iterator it = listChannels.begin();
            while (it != listChannels.end())
            {
                it->second->removeClient(listClients[i]);
                
                if (it->second->getClients().empty())
                {
                    delete it->second;
                    std::map<std::string, Channel*>::iterator toErase = it;
                    ++it;
                    listChannels.erase(toErase);
                }
                else
                {
                    ++it;
                }
            }

            delete listClients[i];
            listClients.erase(listClients.begin() + i);
            break;
        }
    }
    }

/*
从某个客户端 socket 里读数据，拼到它自己的缓冲区里，
按 \r\n 分割成一条一条完整命令，再逐条处理。
*/
void Server::handle_client_data(int client_fd)
{
    char buf[512];//临时存一次 recv() 读到的数据！不是最终数据仓库
    memset(buf, 0, 512);
    
    Client *client = getClientByFd(client_fd);//找到这个 fd 对应的 Client 🪪
    if (!client) return;//没有 Client， 说明内部状态不一致， 直接 return 防崩

    int n = recv(client_fd, buf, 512, 0);//从 socket 读数据
    /*
    n >0 读到数据
    n == 0 客户端正常断开
    n < 0 发生错误
    */
    if (n == 0) {
        disconnectClient(client_fd);
        return;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;
        disconnectClient(client_fd);
        return;
    }

    //把数据拼进“专属缓冲区”
    //每个客户端一个 buffer, recv 到多少，就追加多少
    client->getBuffer().append(buf, n);
    
    size_t pos;
    //std::string::npos = (一个特殊的“永远不可能是正常位置”的值)没找到。
    //“在 buffer 里，能不能找到 \r\n？
    //只要缓冲区里还有一条完整命令，我就继续拆。
    /*
    while (buffer 中还有 "\r\n")
    {
        剪下一条完整消息//substr 拿走要用的，
        删除它//erase 清掉已经用过的。
        继续处理下一条
    }
    */
    while ((pos = client->getBuffer().find("\r\n")) != std::string::npos)
    {
        //pos = buffer.find("\r\n"); // pos = 5
        //buffer: H E L L O \r \n W O R L D ...
        //index:  0 1 2 3 4 5
        //msg = "HELLO"（不包含 \r\n， 这是“一条干净的命令”）
        //从第 0 个字符开始，取 pos 个字符
        std::string msg = client->getBuffer().substr(0, pos);
        
        //从 buffer 的第 0 个位置开始，删除 pos + 2 个字符(+2 是因为要把 \r\n 也删掉)
        //删除后 buffer 变成->  from "HELLO\r\nWORLD\r\nABC"-> "WORLD\r\nABC"
        client->getBuffer().erase(0, pos + 2);
        if (!msg.empty())
            process_command(client, msg);
    }
    /*
    recv 只是“拿字节”，
    buffer 才是“拼消息”，
    \r\n 才是“消息边界”。
    */
}

void Server::process_command(Client *client, const std::string &message)
{
    std::cout << YELLOW << "Processing command from client " << client->getfd() << ": " << message << RESET << std::endl << std::endl;
    std::stringstream ss(message);
    std::string command;
    ss >> command;//>> 这个操作符, 从输入流里读单词

    std::vector<std::string> args;
    std::string arg;
    
    std::string rest;
    std::getline(ss, rest);
    if (!rest.empty() && rest[0] == ' ')
        rest = rest.substr(1);
        //substr(1)=从第 1 个位置开始，一直取到字符串结尾。
        
    size_t colon_pos = rest.find(':');
    if (colon_pos != std::string::npos)
    {
        std::string before_colon = rest.substr(0, colon_pos);
        std::string after_colon = rest.substr(colon_pos + 1);
        
        std::stringstream before_ss(before_colon);
        while(before_ss >> arg)
        {
            args.push_back(arg);
        }
        if (command == "NICK" && colon_pos == 0)
            args.push_back(":" + after_colon);
        else
            args.push_back(after_colon);
    }
    else
    {
        std::stringstream rest_ss(rest);
        while(rest_ss >> arg)
        {
            args.push_back(arg);
        }
    }

    if (command == "PASS")
        handle_pass(this, client, args);
    else if (command == "NICK")
        handle_nick(this, client, args);
    else if (command == "USER")
        handle_user(this, client, args);
    else if (command == "JOIN")
        handle_join(this, client, args);
    else if (command == "PART")
        handle_part(this, client, args);
    else if (command == "PRIVMSG")
        handle_privmsg(this, client, args);
    else if (command == "KICK")
        handle_kick(this, client, args);
    else if (command == "INVITE")
        handle_invite(this, client, args);
    else if (command == "TOPIC")
        handle_topic(this, client, args);
    else if (command == "MODE")
        handle_mode(this, client, args);
    else if (command == "CAP")
        handle_cap(this, client, args);
    else if (command == "PING")
        handle_ping(this, client, args);
    else if (command == "WHOIS")
        handle_whois(this, client, args);
    else if (command == "QUIT")
        handle_quit(this, client, args);
    else if (command == "WHO")
        handle_who(this, client, args);
    else
    {
        std::cout << RED << "Unknown command from client " << client->getfd() << ": " << command << RESET << std::endl << std::endl;
        client->send_reply(":" + _serverName + " 421 " + (client->getname().empty() ? "*" : client->getname()) + " " + command + " :Unknown command");
    }
}

const std::string& Server::getPassword() const
{
    return _password;
}

Client* Server::getClientByFd(int fd)
{
    for (size_t i = 0; i < listClients.size(); ++i)
    {
        if (listClients[i]->getfd() == fd)
            return listClients[i];
    }
    return NULL;
}

Client* Server::getClientByNick(const std::string& nick)
{
    for (size_t i = 0; i < listClients.size(); ++i)
    {
        if (listClients[i]->getname() == nick)
            return listClients[i];
    }
    return NULL;
}

Channel* Server::getChannel(const std::string& name)
{
    std::map<std::string, Channel*>::iterator it = listChannels.find(name);
    if (it != listChannels.end())
        return it->second;
    return NULL;
}

const std::map<std::string, Channel*>& Server::getChannels() const
{
    return listChannels;
}

void Server::createChannel(const std::string& name, Client* op)
{
    Channel *newChannel = new Channel(name, op);
    listChannels.insert(std::make_pair(name, newChannel));
}

const std::string& Server::getServerName() const
{
    return _serverName;
}
