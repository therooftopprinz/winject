#ifndef __UDP_HPP__
#define __UDP_HPP__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <cstring>
#include <string>
#include <stdexcept>

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
    virtual ssize_t sendto(const bfc::ConstBufferView& pData, const Ip4Port& pAddr, int pFlags=0) = 0;
    virtual ssize_t recvfrom(bfc::BufferView& pData, Ip4Port& pAddr, int pFlags=0) = 0;
    virtual int setsockopt(int pLevel, int pOptionName, const void *pOptionValue, socklen_t pOptionLen) = 0;
    virtual int handle() = 0 ;
};

struct IUdpFactory
{
    virtual std::unique_ptr<ISocket> create() = 0;
};

class UdpSocket : public ISocket
{
public:
    UdpSocket()
        : mSockFd(socket(AF_INET, SOCK_DGRAM, 0))
    {
        if (mSockFd  < 0)
        {
            std::string err = std::string("Socket creation failed! Error: ") + strerror(errno);
            throw std::runtime_error(err);
        }
    }

    ~UdpSocket()
    {
        if (mSockFd>=0)
        {
            close(mSockFd);
        }
    }

    int bind(const Ip4Port& pAddr)
    {
        sockaddr_in myaddr;
        memset((char *)&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(pAddr.addr);
        myaddr.sin_port = htons(pAddr.port);

        return ::bind(mSockFd, (sockaddr *)&myaddr, sizeof(myaddr));
    }

    ssize_t sendto(const bfc::ConstBufferView& pData, const Ip4Port& pAddr, int pFlags=0)
    {
        sockaddr_in to;
        to.sin_family = AF_INET;
        to.sin_addr.s_addr = htonl(pAddr.addr);
        to.sin_port = htons(pAddr.port);
        return ::sendto(mSockFd, pData.data(), pData.size(), pFlags, (sockaddr*)&to, sizeof(to));
    }

    ssize_t recvfrom(bfc::BufferView& pData, Ip4Port& pAddr, int pFlags=0)
    {
        sockaddr_in framddr;
        socklen_t framddrSz = sizeof(framddr);
        auto sz = ::recvfrom(mSockFd, pData.data(), pData.size(), pFlags, (sockaddr*)&framddr, &framddrSz);
        pAddr.addr = ntohl(framddr.sin_addr.s_addr);
        pAddr.port = ntohs(framddr.sin_port);
        return sz;
    }

    int setsockopt(int pLevel, int pOptionName, const void *pOptionValue, socklen_t pOptionLen)
    {
        return ::setsockopt(mSockFd, pLevel, pOptionName, pOptionValue, pOptionLen);
    }

    int handle()
    {
        return mSockFd;
    }

private:
    int mSockFd;
};

class UdpFactory : public IUdpFactory
{
public:
    std::unique_ptr<ISocket> create()
    {
        return std::make_unique<UdpSocket>();
    }
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

#endif // __UDP_HPP__