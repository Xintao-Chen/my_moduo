#pragma once

#include <arpa/inet.h>   //是用来写 IP 地址结构体的，
#include <netinet/in.h>  //是用来处理 IP 地址转换和字节序的。
#include <string>

// 封装socket地址类型
class InetAddress {
    public:
        explicit InetAddress(uint16_t port = 0, std::string ip = "127.0.0.1");
        explicit InetAddress(const sockaddr_in &addr)
            : addr_(addr) {};

        std::string toIp() const;
        std::string toIpPort() const;
        uint16_t toPort() const;

        const sockaddr_in *getSockAddr() const { return &addr_; }
        void setSockAddr(const sockaddr_in &addr) { addr_ = addr; }
    
    private:
        sockaddr_in addr_;
};

