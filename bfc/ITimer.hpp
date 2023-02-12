#ifndef __BFC_ITIMER_HPP__
#define __BFC_ITIMER_HPP__

#include <functional>
#include <chrono>

namespace bfc
{

struct ITimer
{
    virtual int schedule(std::chrono::nanoseconds pDiff, std::function<void()> pCb) = 0;
    virtual void cancel(int) = 0;
};

} // namespace bfc

#endif // __BFC_ITIMER_HPP__
