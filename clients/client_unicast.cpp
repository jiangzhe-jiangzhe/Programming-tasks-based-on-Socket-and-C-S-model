#include "../clients/client.h"
#include "../config/config.h"

int main(int argc, char *argv[])
{
    // 命令行解析
    Config config;
    config.ParseArg(argc, argv);

    Client client;
    client.Init(config.get_udp_port(), config.get_server_ip());

    client.Connect(ClientType::Unicast);

    client.Loop(ClientType::Unicast);

    return 0;
}