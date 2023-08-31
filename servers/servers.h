#ifndef SERVERS_H
#define SERVERS_H

#include <stdio.h>

#include <stdlib.h>
#include <sys/select.h>
#include <netdb.h>
#include <thread>
#include <algorithm>
#include <queue>
#include <mutex>

#include "../thread_pool/thread_pool.h"
#include "../connect/connect.h"

class Server
{
public:
    Server();
    ~Server();

    void Init(int tcp_port, int udp_unicast_port, int opt_linger, int thread_num,
              int timeout, const std::string &multicast_ip);
    void InitThreadPool();
    void EventListen();
    void EventLoop();

private:
    int InitTcp();
    int InitUdp();

    void HandleEvent(int sockfd, int is_tcp);
    void HandleTcpConnection();

    void CloseConnect();

private:
    int maxfd_;
    int udp_listenfd_;
    int tcp_listenfd_;

    int tcp_port_;
    int udp_port_;

    int opt_linger_;
    int connection_timeout_;
    std::string multicast_ip_;

    fd_set master_fds_;
    int user_count_;

    // 线程池相关
    int thread_num_;
    Connect *users_;
    ThreadPool<Connect> *thread_pool_;

    char ipbuf_[128];
};
#endif
