#include "./config.h"

Config::Config()
{
    // tcp端口号,默认8080
    tcp_port_ = 8080;

    // udp端口号,默认8081
    udp_port_ = 8081;

    // 优雅关闭链接，默认使用
    opt_linger_ = 1;

    // 线程池内的线程数量,默认6
    thread_num_ = 6;

    timeout_ = 15;

    server_ip_ = "10.112.3.201";

    multicast_ip_ = "224.0.0.1";
}

Config::~Config()
{
}

void Config::ParseArg(int argc, char *argv[])
{
    int opt;
    const char *str = "p:u:m:o:t:c:s:l:";
    while ((opt = getopt(argc, argv, str)) != -1)
    {
        switch (opt)
        {
        case 'p':
        {
            tcp_port_ = atoi(optarg);
            break;
        }
        case 'u':
        {
            udp_port_ = atoi(optarg);
            break;
        }
        case 'o':
        {
            opt_linger_ = atoi(optarg);
            break;
        }
        case 't':
        {
            thread_num_ = atoi(optarg);
            break;
        }
        case 'c':
        {
            timeout_ = atoi(optarg);
            break;
        }
        case 's':
        {
            server_ip_ = optarg;
            break;
        }
        case 'm':
        {
            multicast_ip_ = optarg;
            break;
        }
        default:
            break;
        }
    }
}

int Config::get_tcp_port()
{
    return tcp_port_;
}

int Config::get_udp_port()
{
    return udp_port_;
}

int Config::get_opt_longer()
{
    return opt_linger_;
}

int Config::get_thread_num()
{
    return thread_num_;
}

int Config::get_timeout()
{
    return timeout_;
}

std::string Config::get_server_ip()
{
    return server_ip_;
}
std::string Config::get_multicast_ip()
{
    return multicast_ip_;
}