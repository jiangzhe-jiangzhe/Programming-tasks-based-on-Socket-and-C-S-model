#include "udp_data.h"

UdpData::UdpData()
{
    client_addr_len = sizeof(address_);
    is_client_ = false;
}

UdpData::~UdpData()
{
}

// 发送消息
int UdpData::SendMessage(int sock, char *msg, int size)
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

    int ret = sendto(sock, packetBuffer, packetSize, 0, (struct sockaddr *)&address_, client_addr_len);
    if (ret <= 0)
    {
        delete[] packetBuffer; // 释放内存
        return -1;
    }
    delete[] packetBuffer; // 释放内存

    return 0;
}

// 接收消息
int UdpData::RecvMessage(int sock, char *msg, int max_size)
{
    return recvfrom(sock, msg, max_size, 0, (struct sockaddr *)&address_, &client_addr_len);
}

void UdpData::HandleMessage(char *msg, int size)
{
    // 小写转大写
    for (int i = 0; i < size; ++i)
    {
        msg[i] = toupper(msg[i]);
    }
}

sockaddr_in *UdpData::get_address()
{
    return &address_;
}

void UdpData::set_address(const sockaddr_in &address)
{
    address_ = address;
    is_client_ = true;
}

void UdpData::ConstructProtocol(MessageProtocol &protocol, const char *msg, int size, char *packetBuffer)
{
    memcpy(protocol.head, HEAD, sizeof(HEAD));
    protocol.size = size;

    memcpy(packetBuffer, &protocol, PROTOCOL_SIZE);
    memcpy(packetBuffer + PROTOCOL_SIZE, msg, size);

    protocol.checksum = CalChecksum(packetBuffer, PROTOCOL_SIZE + size);
    memcpy(protocol.tail, TAIL, sizeof(TAIL));

    memcpy(packetBuffer + PROTOCOL_SIZE + size, ((char *)&protocol) + PROTOCOL_SIZE, PROTOCOL_SIZE);
}

// 计算消息内容的校验和
uint32_t UdpData::CalChecksum(const char *data, int size)
{
    return crc32(0L, (const Bytef *)data, (uInt)size);
}
