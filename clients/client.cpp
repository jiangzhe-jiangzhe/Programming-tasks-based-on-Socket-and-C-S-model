#include "client.h"

Client::Client(/* args */)
{
    port_ = -1;
    server_ip_ = "";
    multicast_ip_ = "";
    client_sockfd_ = -1;
    tcp_data_ = new TcpData(1);
    udp_data_ = new UdpData();
}

Client::~Client()
{
}

void Client::Init(int port, const std::string &server_ip, const std::string &multicast_ip)
{
    multicast_ip_ = multicast_ip;
    Init(port, server_ip);
}

void Client::Init(int port, const std::string &server_ip)
{
    port_ = port;
    server_ip_ = server_ip;
}

void Client::Connect(ClientType client_type)
{
    int res = 0;
    switch (client_type)
    {
    case ClientType::TCP:
        res = InitTcp();
        break;
    case ClientType::Unicast:
        res = InitUnicast();
        break;
    case ClientType::Multicast:
        res = InitMulticast();
        break;
    default:
        break;
    }
    if (res == -1)
    {
        std::cerr << "Failed to initialize client" << std::endl;
        exit(1);
    }
}

void Client::Loop(ClientType client_type)
{
    switch (client_type)
    {
    case ClientType::TCP:
        LoopTcp();
        break;
    case ClientType::Unicast:
        LoopUnicast();
        break;
    case ClientType::Multicast:
        LooptMulticast();
        break;
    default:
        break;
    }
}

int Client::InitTcp()
{
    client_sockfd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sockfd_ == -1)
    {
        perror("socket error");
        return -1;
    }

    memset(&srv_addr_, 0, sizeof(srv_addr_));
    srv_addr_.sin_family = AF_INET;
    srv_addr_.sin_port = htons(port_);
    srv_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());

    int ret = connect(client_sockfd_, (struct sockaddr *)&srv_addr_, sizeof(srv_addr_));
    if (ret == -1)
    {
        perror("connect error");
        return -1;
    }

    std::cout << "Welcome to the server!" << std::endl;
    return 1;
}

void Client::LoopTcp()
{
    while (1)
    {
        memset(&msg_, 0, sizeof(msg_));
        fgets(msg_, sizeof(msg_), stdin);

        if (tcp_data_->SendMessage(client_sockfd_, msg_, strlen(msg_)) < 0)
        {
            std::cout << "Error sending message.\n";
            close(client_sockfd_);
            exit(1);
        }

        std::cout << "Send message: " << msg_ << std::endl;

        int lastPos = 0;
        int sumDataLength = 0;
        int nRemainSize = 0;
        char recvbuf[BUFFER_SIZE];
        char databuf[BUFFER_SIZE];
        memset(recvbuf, 0, sizeof(recvbuf));
        memset(databuf, 0, sizeof(databuf));
        memset(&msg_, 0, sizeof(msg_));
        // 接收协议头
        MessageProtocol protocol;

        int recv_len = tcp_data_->RecvMessage(client_sockfd_, recvbuf, sizeof(recvbuf));
        if (recv_len <= 0)
        {
            std::cout << "Error receiving message or connect close.\n";
            close(client_sockfd_);
            exit(1);
        }

        memcpy(databuf + lastPos, recvbuf, recv_len);
        lastPos += recv_len;

        // 判断消息缓冲区的数据长度大于消息头
        while (lastPos >= PROTOCOL_SIZE)
        {
            memcpy(&protocol, databuf, PROTOCOL_SIZE);
            if (strcmp(protocol.head, HEAD) == 0)
            {
                sumDataLength = PROTOCOL_SIZE * 2 + protocol.size;
                // 判断消息缓冲区的数据长度大于消息体
                if (lastPos >= sumDataLength)
                {
                    memcpy(((char *)&protocol) + PROTOCOL_SIZE, databuf + protocol.size + PROTOCOL_SIZE, PROTOCOL_SIZE);
                    // CRC校验
                    if (dynamic_cast<TcpData *>(tcp_data_)->CalChecksum(databuf, protocol.size + PROTOCOL_SIZE) == protocol.checksum && strcmp(protocol.tail, TAIL) == 0)
                    {
                        memcpy(msg_, databuf + PROTOCOL_SIZE, protocol.size);

                        // 处理数据
                        std::cout << "Received  from " << inet_ntoa(srv_addr_.sin_addr) << ":" << ntohs(srv_addr_.sin_port);
                        std::cout << " str:" << msg_ << std::endl;
                        // 剩余未处理消息缓冲区数据的长度
                        nRemainSize = lastPos - sumDataLength;

                        // 将未处理的数据前移
                        if (nRemainSize > 0)
                        {
                            memcpy(databuf, databuf + sumDataLength, nRemainSize);
                        }
                        lastPos = nRemainSize;
                    }
                    else
                    {
                        if (nRemainSize > 0)
                        {
                            memcpy(databuf, databuf + sumDataLength, nRemainSize);
                        }
                        lastPos = nRemainSize;
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                bool isFind = false;
                int nFindStart = 0;
                for (int k = 1; k < lastPos; k++)
                {
                    memcpy(&protocol, databuf + k, PROTOCOL_SIZE);
                    if (strcmp(protocol.head, HEAD) == 0)
                    {
                        nFindStart = k;
                        isFind = true;
                        break;
                    }
                }
                if (isFind == true)
                {
                    memcpy(databuf, databuf + nFindStart, lastPos - nFindStart);
                    lastPos = lastPos - nFindStart;
                }
                else
                {
                    memset(databuf, 0, sizeof(databuf));
                    lastPos = 0;
                    break;
                }
            }
        }
    }
    close(client_sockfd_);
}

int Client::InitUnicast()
{
    // 建立套接字
    client_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0); // IPV4,数据报套接字类型,不指定协议
    if (client_sockfd_ < 0)
    {
        perror("socket");
        return -1;
    }

    memset(&srv_addr_, 0, sizeof(srv_addr_));
    srv_addr_.sin_family = AF_INET;
    srv_addr_.sin_port = htons(port_);
    srv_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
    return 1;
}

void Client::LoopUnicast()
{
    while (1)
    {
        memset(&msg_, 0, sizeof(msg_));
        dynamic_cast<UdpData *>(udp_data_)->set_address(srv_addr_);
        fgets(msg_, sizeof(msg_), stdin);
        if (udp_data_->SendMessage(client_sockfd_, msg_, strlen(msg_)) < 0)
        {
            perror("sendto ERROR");
            break;
        }
        printf("udp client send:[%s - %d]:%s\n", server_ip_.c_str(), port_, msg_);

        ParseAndSendMsg(client_sockfd_, 0);
    }

    // 关闭套接字
    close(client_sockfd_);
}

int Client::InitMulticast()
{
    // 创建UDP套接字
    client_sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_sockfd_ < 0)
    {
        perror("socket");
        return -1;
    }

    /* Enable address reuse */
    int on = 1;
    if (setsockopt(client_sockfd_, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
        close(client_sockfd_);
        return -1;
    }

    if (setsockopt(client_sockfd_, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0)
    {
        perror("setsockopt SO_BROADCAST");
        return -1;
    }

    // 设置IP_MULTICAST_TTL选项为2
    int ttl = 2;
    if (setsockopt(client_sockfd_, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("setsockopt IP_MULTICAST_TTL");
        close(client_sockfd_);
        return -1;
    }

    // 设置接收地址和端口号
    srv_addr_.sin_family = AF_INET;
    srv_addr_.sin_addr.s_addr = inet_addr(multicast_ip_.c_str());
    srv_addr_.sin_port = htons(port_);

    // 绑定到套接字上
    if (bind(client_sockfd_, (struct sockaddr *)&srv_addr_, sizeof(srv_addr_)) < 0)
    {
        perror("bind");
        close(client_sockfd_);
        return -1;
    }

    // 加入组播组
    mreq_.imr_multiaddr.s_addr = inet_addr(multicast_ip_.c_str());
    mreq_.imr_interface.s_addr = htonl(INADDR_ANY);
    if (setsockopt(client_sockfd_, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq_, sizeof(mreq_)) < 0)
    {
        perror("setsockopt IP_ADD_MEMBERSHIP");
        close(client_sockfd_);
        return -1;
    }
    return 1;
}

void Client::LooptMulticast()
{
    // 创建发送线程和接收线程
    std::thread send_t(&Client::send_thread, this, client_sockfd_);
    std::thread recv_t(&Client::recv_thread, this, client_sockfd_);
    // 等待线程结束
    send_t.join();
    recv_t.join();

    close(client_sockfd_);
}

void Client::send_thread(int sock)
{
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(multicast_ip_.c_str());
    addr.sin_port = htons(port_);

    while (true)
    {
        memset(&msg_, 0, sizeof(msg_));
        fgets(msg_, sizeof(msg_), stdin);
        dynamic_cast<UdpData *>(udp_data_)->set_address(addr);
        if (udp_data_->SendMessage(sock, msg_, strlen(msg_)) < 0)
        {
            perror("sendto ERROR");
            break;
        }
    }
}

void Client::recv_thread(int sock)
{
    struct sockaddr_in from;
    while (true)
    {
        ParseAndSendMsg(sock, 1);
    }
}

void Client::ParseAndSendMsg(int sockfd, int is_multicast)
{
    int sumDataLength = 0;
    MessageProtocol protocol;

    memset(&databuf_, 0, sizeof(databuf_));
    memset(&msg_, 0, sizeof(msg_));

    if (udp_data_->RecvMessage(sockfd, msg_, sizeof(msg_)) < 0)
    {
        perror("recvfrom ERROR");
        return;
    }
    struct sockaddr_in from = *dynamic_cast<UdpData *>(udp_data_)->get_address();

    memcpy(&protocol, msg_, PROTOCOL_SIZE);

    if (strcmp(protocol.head, HEAD) == 0)
    {
        sumDataLength = PROTOCOL_SIZE * 2 + protocol.size;

        memcpy(((char *)&protocol) + PROTOCOL_SIZE, msg_ + protocol.size + PROTOCOL_SIZE, PROTOCOL_SIZE);
        // CRC校验
        if (dynamic_cast<UdpData *>(udp_data_)->CalChecksum(msg_, protocol.size + PROTOCOL_SIZE) == protocol.checksum && strcmp(protocol.tail, TAIL) == 0)
        {
            memcpy(databuf_, msg_ + PROTOCOL_SIZE, protocol.size);

            std::cout << "Received  from " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port);
            std::cout << " str:" << databuf_ << std::endl;
        }
        else
        {
            std::cout << "Received  from " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port);
            std::cout << " Received message: invalid" << std::endl;
        }
    }
    else
    {
        std::cout << "Received  from " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port);
        std::cout << " Received message: invalid" << std::endl;
    }
}