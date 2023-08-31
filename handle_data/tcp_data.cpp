#include "tcp_data.h"
#include "../servers/servers.h"

TcpData::TcpData(bool is_client)
{
    memset(ip_buffer_, 0, sizeof(IP_BUFFER));
    memset(buffer_, 0, sizeof(BUFFER_SIZE));
    memset(header_, 0, sizeof(PROTOCOL_SIZE));
    is_client_ = is_client;
}

TcpData::~TcpData()
{
}

// 计算消息内容的校验和
uint32_t TcpData::CalChecksum(const char *data, int size)
{
    return crc32(0L, (const Bytef *)data, (uInt)size);
}

// 发送消息
int TcpData::SendMessage(int sock, char *msg, int size)
{
    if (!is_client_)
    {
        HandleMessage(msg, size);
    }
    // 计算整个数据包的大小，包括尾部标志 , 分配缓冲区
    int packetSize = PROTOCOL_SIZE * 2 + size;
    char *packetBuffer = new char[packetSize];

    // 构造协议
    MessageProtocol protocol;
    ConstructProtocol(protocol, msg, size, packetBuffer);

    int ret = send(sock, packetBuffer, packetSize, 0);
    if (ret <= 0)
    {
        if (errno == EPIPE)
        {
            std::cout << "连接已关闭。\n";
        }
        delete[] packetBuffer; // 释放内存
        return -1;
    }

    delete[] packetBuffer; // 释放内存

    return 0;
}

// 接收消息并且处理了粘包
int TcpData::RecvMessage(int sock, char *msg, int max_size)
{
    return recv(sock, msg, BUFFER_SIZE, 0);
}

void TcpData::HandleMessage(char *msg, int size)
{
    // 小写转大写
    for (int i = 0; i < size; ++i)
    {
        msg[i] = toupper(msg[i]);
    }
}

void TcpData::ConstructProtocol(MessageProtocol &protocol, const char *msg, int size, char *packetBuffer)
{
    memcpy(protocol.head, HEAD, sizeof(HEAD));
    protocol.size = size;

    memcpy(packetBuffer, &protocol, PROTOCOL_SIZE);
    memcpy(packetBuffer + PROTOCOL_SIZE, msg, size);

    protocol.checksum = CalChecksum(packetBuffer, PROTOCOL_SIZE + size);
    memcpy(protocol.tail, TAIL, sizeof(TAIL));
    
    memcpy(packetBuffer + PROTOCOL_SIZE + size, ((char *)&protocol) + PROTOCOL_SIZE, PROTOCOL_SIZE);
}
