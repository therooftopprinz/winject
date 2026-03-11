#ifndef __WINJECTUM_STUBTIMER_HPP__
#define __WINJECTUM_STUBTIMER_HPP__

#include <bfc/timer.hpp>

struct StubMockTimer : bfc::timer<>
{
    using base_t = bfc::timer<>;

    // Expose a helper to trigger the next ready callbacks in tests
    void run_once(int64_t now_us = base_t::current_time_us())
    {
        schedule(now_us);
    }
};

#endif // __WINJECTUM_STUBTIMER_HPP__
