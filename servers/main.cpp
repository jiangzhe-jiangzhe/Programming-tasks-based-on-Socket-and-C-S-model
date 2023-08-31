#include "../servers/servers.h"
#include "../config/config.h"

int main(int argc, char *argv[])
{
    // 命令行解析
    Config config;
    config.ParseArg(argc, argv);

    Server server;

    // 初始化
    server.Init(config.get_tcp_port(), config.get_udp_port(), config.get_opt_longer(),
                config.get_thread_num(), config.get_timeout(), config.get_multicast_ip());

    // 线程池
    server.InitThreadPool();

    // 监听
    server.EventListen();

    // 运行
    server.EventLoop();

    return 0;
}
