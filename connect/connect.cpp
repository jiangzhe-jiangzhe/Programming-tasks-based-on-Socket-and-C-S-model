#include "connect.h"
#include "../servers/servers.h"
#include <fstream>
#include <iostream>
#include <climits>

Connect::Connect()
{
    memset(ip_buffer_, 0, sizeof(IP_BUFFER));
    tcp_data_ = new TcpData(0);
    udp_data_ = new UdpData();
    last_action_time_ = 0;
    recv_len_ = -1;
    sockfd_ = -1;
}

Connect::~Connect()
{
    delete udp_data_;
    delete tcp_data_;
}

// 初始化连接,外部调用初始化套接字地址
void Connect::Init(int sockfd, time_t last_action_time, const sockaddr_in &addr)
{
    address_ = addr;
    Init(sockfd, last_action_time);
}

void Connect::Init(int sockfd, time_t last_action_time, int udp_port)
{
    udp_port_ = udp_port;
    Init(sockfd, last_action_time);
}

void Connect::Init(int sockfd, time_t last_action_time)
{
    sockfd_ = sockfd;
    last_action_time_ = last_action_time;
}

int Connect::HandleRecv()
{
    if (last_action_time_ < 1)
    {
        return HandleUdpRecv();
    }
    else
    {
        return HandleTcpRecv();
    }
}

void Connect::HandleSend()
{
    if (last_action_time_ < 1)
    {
        HandleUdpSend();
    }
    else
    {
        HandleTcpSend();
    }
}

int Connect::HandleTcpRecv()
{
    memset(recvbuf_, 0, sizeof(recvbuf_));
    memset(databuf_, 0, sizeof(databuf_));
    memset(&msg_, 0, sizeof(msg_));
    recv_len_ = tcp_data_->RecvMessage(sockfd_, recvbuf_, sizeof(recvbuf_));
    return recv_len_;
}

void Connect::HandleTcpSend()
{
    int lastPos = 0;
    int sumDataLength = 0;
    int nRemainSize = 0;
    // 接收协议头
    MessageProtocol protocol;

    memcpy(databuf_ + lastPos, recvbuf_, recv_len_);
    lastPos += recv_len_;

    // 判断消息缓冲区的数据长度大于消息头
    while (lastPos >= PROTOCOL_SIZE)
    {
        memcpy(&protocol, databuf_, PROTOCOL_SIZE);
        if (strcmp(protocol.head, HEAD) == 0)
        {
            sumDataLength = PROTOCOL_SIZE * 2 + protocol.size;
            // 判断消息缓冲区的数据长度大于消息体
            if (lastPos >= sumDataLength)
            {
                memcpy(((char *)&protocol) + PROTOCOL_SIZE, databuf_ + protocol.size + PROTOCOL_SIZE, PROTOCOL_SIZE);
                // CRC校验
                if (dynamic_cast<TcpData *>(tcp_data_)->CalChecksum(databuf_, protocol.size + PROTOCOL_SIZE) == protocol.checksum && strcmp(protocol.tail, TAIL) == 0)
                {
                    memcpy(msg_, databuf_ + PROTOCOL_SIZE, protocol.size);

                    // 处理数据
                    std::cout << "Received  from " << inet_ntoa(address_.sin_addr) << ":" << ntohs(address_.sin_port);
                    std::cout << " message: " << msg_ << std::endl;

                    if (tcp_data_->SendMessage(sockfd_, msg_, protocol.size) < 0)
                    {
                        close(sockfd_);
                        perror("Error sending message.");
                    }
                    last_action_time_ = time(0);
                    std::cout << "Sended  to " << inet_ntoa(address_.sin_addr) << ":" << ntohs(address_.sin_port);
                    std::cout << " message: " << msg_ << std::endl;

                    // 剩余未处理消息缓冲区数据的长度
                    nRemainSize = lastPos - sumDataLength;

                    // 将未处理的数据前移
                    if (nRemainSize > 0)
                    {
                        memcpy(databuf_, databuf_ + sumDataLength, nRemainSize);
                    }
                    lastPos = nRemainSize;
                }
                else
                {
                    if (nRemainSize > 0)
                    {
                        memcpy(databuf_, databuf_ + sumDataLength, nRemainSize);
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
                memcpy(&protocol, databuf_ + k, PROTOCOL_SIZE);
                if (strcmp(protocol.head, HEAD) == 0)
                {
                    nFindStart = k;
                    isFind = true;
                    break;
                }
            }
            if (isFind == true)
            {
                memcpy(databuf_, databuf_ + nFindStart, lastPos - nFindStart);
                lastPos = lastPos - nFindStart;
            }
            else
            {
                memset(databuf_, 0, sizeof(databuf_));
                lastPos = 0;
                break;
            }
        }
    }
}

int Connect::HandleUdpRecv()
{
    memset(databuf_, 0, sizeof(databuf_));
    memset(&msg_, 0, sizeof(msg_));
    int recv_len = udp_data_->RecvMessage(sockfd_, databuf_, sizeof(databuf_));
    if (recv_len < 0)
    {
        perror("recvfrom error");
        return recv_len;
    }
    address_ = *dynamic_cast<UdpData *>(udp_data_)->get_address();
    return recv_len;
}

void Connect::HandleUdpSend()
{

    int sumDataLength = 0;
    // 接收协议头
    MessageProtocol protocol;

    memcpy(&protocol, databuf_, PROTOCOL_SIZE);
    if (strcmp(protocol.head, HEAD) == 0)
    {
        sumDataLength = PROTOCOL_SIZE * 2 + protocol.size;

        memcpy(((char *)&protocol) + PROTOCOL_SIZE, databuf_ + protocol.size + PROTOCOL_SIZE, PROTOCOL_SIZE);
        // CRC校验
        if (dynamic_cast<UdpData *>(udp_data_)->CalChecksum(databuf_, protocol.size + PROTOCOL_SIZE) == protocol.checksum && strcmp(protocol.tail, TAIL) == 0)
        {
            memcpy(msg_, databuf_ + PROTOCOL_SIZE, protocol.size);

            if (udp_port_ != ntohs(address_.sin_port))
            {
                printf("Received unicast  UDP from client iP: %s, port: %d, str = %s\n", inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip_buffer_, sizeof(ip_buffer_)), ntohs(address_.sin_port), msg_);

                if (udp_data_->SendMessage(sockfd_, msg_, protocol.size) < 0)
                {
                    perror("Error sending message.");
                    return;
                }
                printf("Sended unicast  UDP to client iP: %s, port: %d, str = %s\n", inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip_buffer_, sizeof(ip_buffer_)), ntohs(address_.sin_port), msg_);
            }
            else
            {
                printf("Received multicast UDP from client iP: %s, port: %d, str = %s\n", inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip_buffer_, sizeof(ip_buffer_)), ntohs(address_.sin_port), msg_);
            }
        }
        else
        {
            printf("Received from client iP: %s, port: %d", inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip_buffer_, sizeof(ip_buffer_)), ntohs(address_.sin_port));
            std::cout << "Received message: invalid" << std::endl;
        }
    }
    else
    {
        printf("Received from client iP: %s, port: %d", inet_ntop(AF_INET, &address_.sin_addr.s_addr, ip_buffer_, sizeof(ip_buffer_)), ntohs(address_.sin_port));
        std::cout << "Received message: invalid" << std::endl;
    }
}

sockaddr_in *Connect::get_address()
{
    return &address_;
}

void Connect::set_address(sockaddr_in *address)
{
    address_ = *address;
}

int Connect::get_sockfd()
{
    return sockfd_;
}

void Connect::set_sockfd(int sockfd)
{
    sockfd_ = sockfd;
}

std::atomic<time_t> *Connect::get_last_action_time()
{
    return &last_action_time_;
}

void Connect::set_last_action_time(int last_action_time)
{
    last_action_time_ = last_action_time;
}
