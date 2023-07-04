#ifndef __BFC_ITIMER_HPP__
#define __BFC_ITIMER_HPP__

#include <functional>
#include <chrono>

namespace bfc
{

struct ITimer
{
    virtual uint64_t schedule(std::chrono::nanoseconds pDiff, std::function<void()> pCb) = 0;
    virtual void cancel(uint64_t) = 0;
};

} // namespace bfc

#endif // __BFC_ITIMER_HPP__
