#ifndef __WINJECTUM_DUALWIFI_HPP__
#define __WINJECTUM_DUALWIFI_HPP__

#include <sstream>
#include <winject/wifi.hpp>
#include <winject/802_11_filters.hpp>
#include "IWIFI.hpp"

class DualWIFI : public IWIFI
{
public:
    DualWIFI() = delete;
    DualWIFI(std::string tx, std::string rx)
        : tx(tx)
        , rx(rx)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000*100;
        setsockopt(this->rx, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }

    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sendto(tx, buff, sz, 0, (sockaddr *) &tx.address(), sizeof(struct sockaddr_ll));
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        return ::recv(rx, buffer, sz, 0);
    }

    int handle()
    {
        return rx;
    }

private:
    winject::wifi tx;
    winject::wifi rx;
};

#endif // __WINJECTUM_DUALWIFI_HPP__
