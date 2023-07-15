#ifndef __BFC_UDP_HPP__
#define __BFC_UDP_HPP__

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>
#include <cstring>
#include <string>
#include <stdexcept>

#include "Buffer.hpp"
#include "ISocket.hpp"

namespace bfc
{
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

    int connect(const Ip4Port& pAddr) override
    {
        errno = EFAULT;
        return -1;
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

    ssize_t send(const bfc::ConstBufferView& pData, int pFlags=0)
    {
        errno = EFAULT;
        return-1;
    }
    ssize_t recv(bfc::BufferView& pData, int pFlags=0)
    {
        errno = EFAULT;
        return -1;
    }

    int setsockopt(int pLevel, int pOptionName, const void *pOptionValue, socklen_t pOptionLen)
    {
        return ::setsockopt(mSockFd, pLevel, pOptionName, pOptionValue, pOptionLen);
    }

    int handle()
    {
        return mSockFd;
    }

    operator bool() override
    {
        return mSockFd >= 0;
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

} // namespace bfc

#endif // __BFC_UDP_HPP__
