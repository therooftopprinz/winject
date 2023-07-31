#ifndef __WINJECTUM_STUBTIMER_HPP__
#define __WINJECTUM_STUBTIMER_HPP__

#include "ITimer.hpp"

struct StubMockTimer : ITimer
{
    int schedule(std::chrono::nanoseconds pDiff, std::function<void()> pCb)
    {
        last_cb = pCb;
        return id_ctr++;
    }

    void cancel(int)
    {}

    std::function<void()> last_cb;
    int id_ctr = 0;
};

#endif // __WINJECTUM_STUBTIMER_HPP__
