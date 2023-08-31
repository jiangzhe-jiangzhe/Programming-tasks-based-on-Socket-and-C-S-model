#ifndef TCPDATA_H
#define TCPDATA_H

#include "handle_data.h"

class TcpData : public HandleData
{
public:
    TcpData(bool is_client);
    ~TcpData();

    virtual int SendMessage(int sock, char *msg, int size);
    virtual int RecvMessage(int sock, char *msg, int max_size);
    virtual void HandleMessage(char *msg, int size);
    virtual uint32_t CalChecksum(const char *data, int size);
    virtual void ConstructProtocol(MessageProtocol &protocol, const char *msg, int size, char *packetBuffer);

private:
    char ip_buffer_[IP_BUFFER];
    char buffer_[BUFFER_SIZE];
    char header_[PROTOCOL_SIZE];
    bool is_client_;
};

#endif