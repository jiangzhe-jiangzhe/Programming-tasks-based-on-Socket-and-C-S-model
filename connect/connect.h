#ifndef CONNECTION_H
#define CONNECTION_H
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <map>
#include <atomic>

#include "../lock/locker.h"
#include "../handle_data/handle_data.h"
#include "../handle_data/udp_data.h"
#include "../handle_data/tcp_data.h"

class Connect
{
public:
    Connect();
    ~Connect();

    
    void Init(int sockfd, time_t last_action_time, int udp_port);
    void Init(int sockfd, time_t last_action_time, const sockaddr_in &addr);
    void Init(int sockfd, time_t last_action_time);

    int HandleRecv();
    void HandleSend();

    int get_sockfd();
    std::atomic<time_t> *get_last_action_time();
    sockaddr_in *get_address();

    void set_sockfd(int sockfd);
    void set_address(sockaddr_in *address);
    void set_last_action_time(int last_action_time);

private:
    int HandleTcpRecv();
    void HandleTcpSend();

    void HandleUdpSend();
    int HandleUdpRecv();

private:
    int sockfd_;
    int udp_port_;
    struct sockaddr_in address_;

    HandleData *udp_data_;
    HandleData *tcp_data_;

    std::atomic<time_t> last_action_time_;

    char ip_buffer_[IP_BUFFER];
    char msg_[BUFFER_SIZE];
    char recvbuf_[BUFFER_SIZE];
    char databuf_[BUFFER_SIZE];
    int recv_len_;
};

#endif
