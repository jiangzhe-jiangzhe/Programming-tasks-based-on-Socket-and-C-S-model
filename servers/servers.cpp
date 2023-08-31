#include "servers.h"

#include <iostream>

Server::Server()
{
    user_count_ = 0;
    maxfd_ = 0;
    thread_pool_ = nullptr;
    memset(ipbuf_, 0, sizeof(ipbuf_));
}

Server::~Server()
{
    close(tcp_listenfd_);
    close(udp_listenfd_);
    // close(udp_multicastfd_);
    delete[] users_;
    delete thread_pool_;
}

void Server::Init(int tcp_port, int udp_port,
                  int opt_linger, int thread_num, int timeout, const std::string &multicast_ip)
{
    users_ = new Connect[MAX_FD];
    tcp_port_ = tcp_port;
    udp_port_ = udp_port;
    thread_num_ = thread_num;
    opt_linger_ = opt_linger;
    connection_timeout_ = timeout;
    multicast_ip_ = multicast_ip;
}

int Server::InitTcp()
{
    tcp_listenfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_listenfd_ < 0)
    {
        perror("socket error");
        return -1;
    }

    // 优雅关闭连接
    struct linger tmp = {opt_linger_, 1};
    if (setsockopt(tcp_listenfd_, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp)) < 0)
    {
        perror("setsockopt SO_LINGER");
        return -1;
    }

    struct sockaddr_in tcp_listen_addr;
    memset(&tcp_listen_addr, 0, sizeof(tcp_listen_addr));
    tcp_listen_addr.sin_family = AF_INET;
    tcp_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcp_listen_addr.sin_port = htons(tcp_port_);

    if (bind(tcp_listenfd_, (struct sockaddr *)&tcp_listen_addr, sizeof(tcp_listen_addr)) < 0)
    {
        perror("bind error");
        return -1;
    }

    if (listen(tcp_listenfd_, 10) < 0)
    {
        perror("listen error");
        return -1;
    }

    return 0;
}

int Server::InitUdp()
{
    // 单播
    udp_listenfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_listenfd_ < 0)
    {
        perror("socket error");
        return -1;
    }

    int on = 1;
    if (setsockopt(udp_listenfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
        return -1;
    }

    if (setsockopt(udp_listenfd_, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_BROADCAST");
        return -1;
    }

    struct sockaddr_in udp_listen_addr;
    memset(&udp_listen_addr, 0, sizeof(udp_listen_addr));
    udp_listen_addr.sin_family = AF_INET;
    udp_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_listen_addr.sin_port = htons(udp_port_);

    if (bind(udp_listenfd_, (struct sockaddr *)&udp_listen_addr, sizeof(udp_listen_addr)) < 0)
    {
        perror("bind error");
        return -1;
    }

    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(multicast_ip_.c_str());
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(udp_listenfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
    {
        perror("setsockopt error");
        return -1;
    }

    FD_ZERO(&master_fds_);
    FD_SET(tcp_listenfd_, &master_fds_);
    FD_SET(udp_listenfd_, &master_fds_);
    maxfd_ = std::max(udp_listenfd_, tcp_listenfd_);

    return 0;
}

void Server::InitThreadPool()
{
    // 断开空闲连接的线程也放到这里了
    std::thread close_connect_thread(&Server::CloseConnect, this);
    close_connect_thread.detach();
    thread_pool_ = new ThreadPool<Connect>(thread_num_);
}

void Server::EventListen()
{
    if (InitTcp() != 0)
    {
        std::cerr << "Failed to initialize TCP socket" << std::endl;
        exit(1);
    }
    if (InitUdp() != 0)
    {
        std::cerr << "Failed to initialize UDP sockets" << std::endl;
        exit(1);
    }
}

void Server::EventLoop()
{
    while (true)
    {
        fd_set read_fds = master_fds_;
        int res = select(maxfd_ + 1, &read_fds, NULL, NULL, NULL);
        if (res <= 0)
        {
            perror("select error");
            exit(1);
        }

        for (int i = 0; i <= maxfd_; i++)
        {
            if (FD_ISSET(i, &read_fds))
            {
                if (i == udp_listenfd_) // udp
                {
                    HandleEvent(i, 0);
                }
                else if (i == tcp_listenfd_)
                {
                    HandleTcpConnection();
                }
                else // tcp
                {
                    HandleEvent(i, 1);
                }
            }
        }
    }
}

void Server::HandleEvent(int sockfd, int is_tcp)
{
    int res = INT_MAX;
    if (is_tcp < 1)
    {
        users_[sockfd].Init(sockfd, is_tcp, udp_port_);
    }
    if (thread_pool_->Append(users_ + sockfd, res))
    {
        return;
    }

    if (is_tcp == 1)
    {
        if (res <= 0)
        {
            close(sockfd);
            FD_CLR(sockfd, &master_fds_);
            --user_count_;
            users_[sockfd].set_last_action_time(0);
            std::cout << "Disconnect client iP: " << inet_ntop(AF_INET, &users_[sockfd].get_address()->sin_addr.s_addr, ipbuf_, sizeof(ipbuf_)) << ", port: " << ntohs(users_[sockfd].get_address()->sin_port) << std::endl;
            std::cout << "tcp connect count : " << user_count_ << std::endl;
        }
    }
}

void Server::HandleTcpConnection()
{
    struct sockaddr_in client_addr;
    socklen_t client_addrLen = sizeof(client_addr);

    int newfd = accept(tcp_listenfd_, (struct sockaddr *)&client_addr, &client_addrLen);
    if (newfd < 0)
    {
        perror("accept error");
        return;
    }
    std::cout << "Connect client iP: " << inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, ipbuf_, sizeof(ipbuf_)) << ", port: " << ntohs(client_addr.sin_port) << std::endl;

    FD_SET(newfd, &master_fds_);
    maxfd_ = std::max(maxfd_, newfd);
    if (maxfd_ >= MAX_FD)
    {
        FD_CLR(newfd, &master_fds_);
        printf("%s", "Internal server busy");
        return;
    }

    users_[newfd].Init(newfd, time(0), client_addr);
    ++user_count_;
    std::cout << "tcp connect count : " << user_count_ << std::endl;
}

void Server::CloseConnect()
{
    while (true)
    {
        sleep(connection_timeout_);
        if (user_count_ < 1)
            continue;

        for (int i = 0; i < MAX_FD; ++i)
        {
            // 0是没有连接。非零正整数就是tcp连接的
            if (*users_[i].get_last_action_time() < 1)
                continue;
            if (time(0) - *users_[i].get_last_action_time() > connection_timeout_)
            {
                users_[i].set_last_action_time(INT_MAX);
                int ret = shutdown(users_[i].get_sockfd(), SHUT_RDWR); // 关闭读写连接
                std::cout << "succeed close!" << std::endl;
            }
        }
    }
}