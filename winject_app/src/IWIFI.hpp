#ifndef __WINJECTUM_IWIFI_HPP__
#define __WINJECTUM_IWIFI_HPP__

#include <functional>
#include <chrono>

struct IWIFI
{
    virtual ssize_t send(const uint8_t*, size_t) = 0;
    virtual ssize_t recv(uint8_t*, size_t) = 0;
    virtual int handle() = 0;
};

#endif // __WINJECTUM_IWIFI_HPP__
