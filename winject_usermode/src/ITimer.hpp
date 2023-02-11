#ifndef __WINJECTUM_ITIMER_HPP__
#define __WINJECTUM_ITIMER_HPP__

#include <functional>
#include <chrono>

struct ITimer
{
    virtual int schedule(std::chrono::nanoseconds pDiff, std::function<void()> pCb) = 0;
    virtual void cancel(int) = 0;
};

#endif // __WINJECTUM_ITIMER_HPP__
