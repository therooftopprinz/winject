#ifndef __WINJECTUM_WIFI_HPP__
#define __WINJECTUM_WIFI_HPP__

#include <sstream>
#include <winject/wifi.hpp>
#include <winject/802_11_filters.hpp>
#include "IWIFI.hpp"

class WIFI : public IWIFI
{
public:
    WIFI() = delete;
    WIFI(std::string device)
        : wdev(device)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000*100;
        setsockopt(wdev, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }
    
    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sendto(wdev, buff, sz, 0, (sockaddr *) &wdev.address(), sizeof(struct sockaddr_ll));
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        return ::recv(wdev, buffer, sz, 0);
    }

    int handle()
    {
        return wdev;
    }

private:
    winject::wifi wdev;
};

#endif // __WINJECTUM_WIFI_HPP__
