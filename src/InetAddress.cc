#include <strings.h>
#include <string.h>

#include "InetAddress.h"

InetAddress::InetAddress(uint16_t port, std::string ip) {
    ::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;    //指定地址族（AF_INET 表示 IPv4)
    addr_.sin_port = ::htons(port); // 端口由本地字节序转为网络字节序
    addr_.sin_addr.s_addr = :: inet_addr(ip.c_str()); //放二进制的ip地址   
} 

std::string InetAddress::toIp() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const {
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = ::strlen(buf);
    uint16_t port = ::ntohs(addr_.sin_port);
    sprintf(buf+end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ::ntohs(addr_.sin_port);
}

#if TEST_MODE
#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
}
#endif