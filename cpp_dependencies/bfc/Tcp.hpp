/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * MIT License 
 * 
 */
#ifndef __BFC_TCP_HPP__
#define __BFC_TCP_HPP__

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
struct ITcpFactory
{
    virtual std::unique_ptr<ISocket> create() = 0;
};

class TcpSocket : public ISocket
{
public:
    TcpSocket()
        : mSockFd(socket(AF_INET, SOCK_STREAM, 0))
    {
        if (mSockFd  < 0)
        {
            std::string err = std::string("Socket creation failed! Error: ") + strerror(errno);
            throw std::runtime_error(err);
        }
    }

    TcpSocket(int fd)
        : mSockFd(fd)
    {}

    ~TcpSocket()
    {
        if (mSockFd>=0)
        {
            close(mSockFd);
        }
    }

    TcpSocket& operator=(TcpSocket&& mOther)
    {
        if (mSockFd>=0)
        {
            close(mSockFd);
        }

        mSockFd = mOther.mSockFd;
        mOther.mSockFd = -1;
        return *this;
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

    int connect(const Ip4Port& pAddr)
    {
        sockaddr_in myaddr;
        memset((char *)&myaddr, 0, sizeof(myaddr));
        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(pAddr.addr);
        myaddr.sin_port = htons(pAddr.port);

        return ::connect(mSockFd, (sockaddr *)&myaddr, sizeof(myaddr));
    }

    ssize_t sendto(const bfc::ConstBufferView& pData, const Ip4Port& pAddr, int pFlags=0)
    {
        errno = EFAULT;
        return-1;
    }

    ssize_t recvfrom(bfc::BufferView& pData, Ip4Port& pAddr, int pFlags=0)
    {
        errno = EFAULT;
        return-1;
    }

    ssize_t send(const bfc::ConstBufferView& pData, int pFlags=0)
    {
        return ::send(mSockFd, pData.data(), pData.size(), pFlags);
    }

    ssize_t recv(bfc::BufferView& pData, int pFlags=0)
    {
        return ::recv(mSockFd, pData.data(), pData.size(), pFlags);
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
    int mSockFd=-1;
};

class TcpFactory : public ITcpFactory
{
public:
    std::unique_ptr<ISocket> create()
    {
        return std::make_unique<TcpSocket>();
    }
};

} // namespace bfc

#endif // __BFC_TCP_HPP__
