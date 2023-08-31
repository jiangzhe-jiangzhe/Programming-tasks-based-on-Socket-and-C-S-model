#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <unistd.h>

using namespace std;

class Config
{
public:
    Config();
    ~Config();

    void ParseArg(int argc, char *argv[]);

    int get_tcp_port();
    int get_udp_port();
    int get_opt_longer();
    int get_thread_num();
    int get_timeout();
    std::string get_server_ip();
    std::string get_multicast_ip();

private:
    // tcp端口号,默认8080
    int tcp_port_;

    // udp单播端口号,默认8081
    int udp_port_;

    // 优雅关闭链接
    int opt_linger_;

    // 线程池内的线程数量
    int thread_num_;

    // 超时时间
    int timeout_;

    std::string server_ip_;

    std::string multicast_ip_;
};

#endif