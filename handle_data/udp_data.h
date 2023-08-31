#ifndef UDPDATA_H
#define UDPDATA_H
#include "handle_data.h"

class UdpData : public HandleData
{
public:
    UdpData();
    ~UdpData();

    virtual int SendMessage(int sock, char *msg, int size);
    virtual int RecvMessage(int sock, char *msg, int max_size);
    virtual void HandleMessage(char *msg, int size);
    virtual uint32_t CalChecksum(const char *data, int size);
    virtual void ConstructProtocol(MessageProtocol &protocol, const char *msg, int size, char *packetBuffer);

    sockaddr_in *get_address();
    void set_address(const sockaddr_in &address);

private:
    struct sockaddr_in address_;
    socklen_t client_addr_len;
    bool is_client_;
};
#endif