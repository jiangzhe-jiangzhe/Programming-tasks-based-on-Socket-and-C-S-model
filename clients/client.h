#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include "../handle_data/handle_data.h"
#include "../handle_data/udp_data.h"
#include "../handle_data/tcp_data.h"

enum ClientType
{
    TCP = 0,
    Unicast,
    Multicast
};

class Client
{
public:
    Client(/* args */);
    ~Client();

    void Init(int port, const std::string &server_ip_);
    void Init(int port, const std::string &server_ip_, const std::string &multicast_ip);

    void Connect(ClientType client_type);
    void Loop(ClientType client_type);

private:
    int InitTcp();
    int InitUnicast();
    int InitMulticast();

    void LoopTcp();
    void LoopUnicast();
    void LooptMulticast();

    void ParseAndSendMsg(int sockfd, int is_multicast);
    
    void send_thread(int sock);
    void recv_thread(int sock);

private:
    int port_;
    int client_sockfd_;
    std::string server_ip_;
    std::string multicast_ip_;

    struct sockaddr_in srv_addr_;
    struct ip_mreq mreq_;

    HandleData *udp_data_;
    HandleData *tcp_data_;

    char msg_[BUFFER_SIZE];
    char databuf_[BUFFER_SIZE];
};

#endif