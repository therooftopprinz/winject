/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * MIT License 
 * 
 */

#ifndef __BFC_ISOCKET_HPP__
#define __BFC_ISOCKET_HPP__

#include <cstdint>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "Buffer.hpp"

namespace bfc
{

struct Ip4Port
{
    Ip4Port() = default;
    Ip4Port (uint32_t pAddr, uint16_t pPort)
        : addr(pAddr)
        , port(pPort)
    {}

    uint32_t addr;
    uint16_t port;
};

struct ISocket
{
    virtual int bind(const Ip4Port& pAddr) = 0;
    virtual int connect(const Ip4Port& pAddr) = 0;
    virtual ssize_t sendto(const bfc::ConstBufferView& pData, const Ip4Port& pAddr, int pFlags=0) = 0;
    virtual ssize_t recvfrom(bfc::BufferView& pData, Ip4Port& pAddr, int pFlags=0) = 0;
    virtual ssize_t send(const bfc::ConstBufferView& pData, int pFlags=0) = 0;
    virtual ssize_t recv(bfc::BufferView& pData, int pFlags=0) = 0;
    virtual int setsockopt(int pLevel, int pOptionName, const void *pOptionValue, socklen_t pOptionLen) = 0;
    virtual int handle() = 0;
    virtual operator bool() = 0;
};

inline Ip4Port toIp4Port(uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint16_t port)
{
    return Ip4Port(uint32_t((a<<24)|(b<<16)|(c<<8)|d), port);
}

inline Ip4Port toIp4Port(std::string host, int port)
{
    sockaddr_in sa;
    inet_pton(AF_INET, host.c_str(), &(sa.sin_addr));
    return Ip4Port(htonl(sa.sin_addr.s_addr), port);
}

inline Ip4Port toIp4Port(std::string hostport)
{
    std::stringstream ss(hostport);
    std::string line;
    std::vector<std::string> parts;

    while(std::getline(ss, line, ':'))
    {
        parts.push_back(line);
    }

    if (parts.size()!=2)
    {
        return {};
    }

    auto& host = parts[0];
    auto port = std::stoul(parts[1]);

    sockaddr_in sa;
    inet_pton(AF_INET, host.c_str(), &(sa.sin_addr));
    return Ip4Port(htonl(sa.sin_addr.s_addr), port);
}

} // namespace bfc

#endif // __BFC_ISOCKET_HPP__
