#ifndef HANDLEDATA_H
#define HANDLEDATA_H

#include <stdint.h>
#include <zlib.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>

const int MAX_FD = 1000; // 最大文件描述符
const int IP_BUFFER = 128;
const int BUFFER_SIZE = 1024;
const int PROTOCOL_SIZE = 8;
#define HEAD "InI"
#define TAIL "EnD"

#pragma pack(1) // 编译器将按照1个字节对齐。
struct MessageProtocol
{
    char head[4];
    int size;
    uint32_t checksum;
    char tail[4];
};
#pragma pack()

class HandleData
{
public:
    virtual int SendMessage(int sock, char *msg, int size) = 0;
    virtual int RecvMessage(int sock, char *msg, int max_size) = 0;
    virtual void HandleMessage(char *msg, int size) = 0;
    virtual uint32_t CalChecksum(const char *data, int size) = 0;
    virtual void ConstructProtocol(MessageProtocol &protocol, const char *msg, int size, char *packetBuffer) = 0;
};

#endif