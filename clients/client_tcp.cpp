#include "../clients/client.h"
#include "../config/config.h"

int main(int argc, char *argv[])
{
    // 命令行解析
    Config config;
    config.ParseArg(argc, argv);

    Client client;
    client.Init(config.get_tcp_port(), config.get_server_ip());

    client.Connect(ClientType::TCP);

    client.Loop(ClientType::TCP);

    return 0;
}