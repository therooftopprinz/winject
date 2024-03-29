/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * MIT License 
 * 
 */

#ifndef __BFC_TIMER_HPP__
#define __BFC_TIMER_HPP__

#include <map>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>

#include "ITimer.hpp"

namespace bfc
{

template <typename FunctorType = std::function<void()>>
class Timer : public ITimer
{
public:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>; 
    Timer()
    {
        mStartRef = std::chrono::high_resolution_clock::now();
    }

    uint64_t schedule(std::chrono::nanoseconds pDiff, FunctorType pCb)
    {
        auto tp = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::mutex> lg(mSchedulesMutex);
        auto id = mIdCtr++;
        auto next = tp + pDiff;

        // [[maybe_unused]] auto printTp = std::chrono::duration_cast<std::chrono::nanoseconds>(tp.time_since_epoch()).count();
        // [[maybe_unused]] auto printNx = std::chrono::duration_cast<std::chrono::nanoseconds>(next.time_since_epoch()).count();
        // [[maybe_unused]] auto printDf = std::chrono::duration_cast<std::chrono::nanoseconds>(pDiff).count();
        // std::cout << "ADD(" << id << ") TPA=" << printTp << " DIFF=" << printDf << " NX=" << printNx << "\n";

        auto res = mSchedules.emplace(next, std::pair(id, std::move(pCb)));
        mScheduleIds.emplace(id, res);
        mAddRequest = true;

        mSchedulesCv.notify_one();
        return id;
    }

    void cancel(uint64_t pId)
    {
        std::unique_lock<std::mutex> lg(mSchedulesMutex);

        if (mToRunId == pId)
        {
            mCancelCurrent = true;
            mSchedulesCv.notify_one();
        }

        auto foundIt = mScheduleIds.find(pId);
        if (mScheduleIds.end() == foundIt)
        {
            return;
        }

        // std::cout << "CANCEL " << pId << "\n";
        auto remIt = foundIt->second;
        mSchedules.erase(remIt);
        mScheduleIds.erase(foundIt);
        mCancelRequest = true;

        mSchedulesCv.notify_one();
    }

    void run()
    {
        mRunning = true;
        auto waitBreaker = [this]() {
                return !mRunning || mCancelRequest || mAddRequest;
            };

        while (mRunning)
        {
            // @note : check cancel request to check if mToRun is removed
            // @note : check add request to check if mToRun is not first
            if (mToRun && !mCancelRequest && !mAddRequest)
            {
                // std::cout << "RUN " << mToRunId << "\n";
                mToRun();
                mToRun = {};
            }
            else if (mToRun && mCancelCurrent)
            {
                mToRun = {};
            }

            std::unique_lock<std::mutex> lg(mSchedulesMutex);
            auto tp = std::chrono::high_resolution_clock::now();

            if (mSchedules.size())
            {
                auto firstIt =  mSchedules.begin();

                auto setToRun = [this](auto it)
                {
                    mToRunTime = it->first;
                    mToRunId = it->second.first;
                    mToRun = std::move(it->second.second);
                    mScheduleIds.erase(it->second.first);
                    mSchedules.erase(it);
                };

                // @note has earlier schedule
                if (mToRun && mToRunTime > firstIt->first)
                {
                    // @note place back current to schedulables
                    if (!mCancelCurrent)
                    {
                        auto res = mSchedules.emplace(mToRunTime, std::pair(mToRunId, std::move(mToRun)));
                        mScheduleIds.emplace(mToRunId, res);
                    }

                    setToRun(firstIt);
                }
                else if(!mToRun)
                {
                    setToRun(firstIt);
                }

                auto tdiff = mToRunTime - tp;
                mAddRequest = false;
                mCancelRequest = false;
                mCancelCurrent = false;
                mSchedulesCv.wait_for(lg, tdiff, waitBreaker);

                // [[maybe_unused]] auto ntp = std::chrono::high_resolution_clock::now();
                // [[maybe_unused]] auto printTp = std::chrono::duration_cast<std::chrono::nanoseconds>(ntp.time_since_epoch()).count();
                // [[maybe_unused]] auto printAwt = std::chrono::duration_cast<std::chrono::nanoseconds>(ntp-tp).count();
                // [[maybe_unused]] auto printIwt = std::chrono::duration_cast<std::chrono::nanoseconds>(tdiff).count();
                // std::cout << "BRK TPR=" << printTp << " IWT=" << printIwt << " AWT=" << printAwt << "\n";
                continue;
            }
            else
            {
                mToRun = {};
                mAddRequest = false;
                mCancelRequest = false;
                mCancelCurrent = false;
            }

            mSchedulesCv.wait(lg, waitBreaker);
        }
    }

    void stop()
    {
        mRunning = false;
        mSchedulesCv.notify_one();
    }

private:
    using ScheduleIdFn = std::pair<uint64_t, FunctorType>;
    using ScheduleMap = std::multimap<TimePoint, ScheduleIdFn>;
    ScheduleMap mSchedules;
    std::unordered_map<uint64_t, typename ScheduleMap::iterator> mScheduleIds;
    uint64_t mIdCtr = 0;

    FunctorType mToRun;
    uint64_t mToRunId;
    TimePoint mToRunTime;
    
    std::condition_variable mSchedulesCv;
    bool mCancelRequest = false;
    bool mCancelCurrent = false;
    bool mAddRequest = false;
    std::mutex mSchedulesMutex;

    TimePoint mStartRef;
    bool mRunning;
};
} // namespace bfc

#endif // __BFC_TIMER_HPP__
