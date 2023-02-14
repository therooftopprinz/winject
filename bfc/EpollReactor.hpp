#ifndef __EpollReactor_HPP__
#define __EpollReactor_HPP__

#include <mutex>
#include <functional>
#include <cstring>

#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/unistd.h>

#include "IReactor.hpp"

namespace bfc
{

class EpollReactor : public IReactor
{
public:
    EpollReactor(const EpollReactor&) = delete;
    void operator=(const EpollReactor&) = delete;

    EpollReactor()
        : mEventCache(mEventCacheSize)
        , mEpollFd(epoll_create1(0))
    {
        if (-1 == mEpollFd)
        {
            throw std::runtime_error(strerror(errno));
        }

        mEventFd = eventfd(0, EFD_SEMAPHORE);

        if (-1 == mEventFd)
        {
            throw std::runtime_error(strerror(errno));
        }

        addReadHandler(mEventFd, [this](){
                uint64_t one;
                static_assert(8 == sizeof(one));
                read(mEventFd, &one, 8);
            });
    }

    ~EpollReactor()
    {
        stop();
        close(mEventFd);
        close(mEpollFd);
    }

    bool addReadHandler(int pFd, Callback pReadCallback)
    {
        std::unique_lock<std::mutex> lgCallback(mCallbackMapMutex);

        bool has_read_handler  = mReadCallbackMap.count(pFd);
        bool has_write_handler = mWriteCallbackMap.count(pFd);

        if (has_read_handler)
        {
            return false;
        }

        epoll_event event{};

        if (has_write_handler)
        {
            event.events |= EPOLLOUT;
        }

        event.data.fd = pFd;
        event.events |= (EPOLLIN | EPOLLRDHUP);

        int res;

        if (has_write_handler)
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, pFd, &event);
        }
        else
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, pFd, &event);
        }

        if (-1 == res)
        {
            mReadCallbackMap.erase(pFd);
            mWriteCallbackMap.erase(pFd);
            epoll_ctl(mEpollFd, EPOLL_CTL_DEL, pFd, nullptr);
            return false;
        }

        mReadCallbackMap.emplace(pFd, std::move(pReadCallback));

        return true;
    }

    bool addWriteHandler(int pFd, Callback pWriteCallback)
    {
        std::unique_lock<std::mutex> lgCallback(mCallbackMapMutex);

        bool has_read_handler  = mReadCallbackMap.count(pFd);
        bool has_write_handler = mWriteCallbackMap.count(pFd);

        if (has_write_handler)
        {
            return false;
        }

        epoll_event event{};

        if (has_read_handler)
        {
            event.events |= (EPOLLIN | EPOLLRDHUP);
        }

        event.data.fd = pFd;
        event.events |= EPOLLOUT;

        int res;

        if (has_read_handler)
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, pFd, &event);
        }
        else
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_ADD, pFd, &event);
        }

        if (-1 == res)
        {
            mReadCallbackMap.erase(pFd);
            mWriteCallbackMap.erase(pFd);
            epoll_ctl(mEpollFd, EPOLL_CTL_DEL, pFd, nullptr);
            return false;
        }

        mWriteCallbackMap.emplace(pFd, std::move(pWriteCallback));

        return true;
    }

    bool removeReadHandler(int pFd)
    {
        std::unique_lock<std::mutex> lgCallback(mCallbackMapMutex);

        bool has_read_handler  = mReadCallbackMap.count(pFd);
        bool has_write_handler = mWriteCallbackMap.count(pFd);

        if (!has_read_handler)
        {
            return false;
        }

        int res;
        epoll_event event{};
        event.data.fd = pFd;
        event.events = EPOLLOUT;

        if (has_write_handler)
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, pFd, &event);
        }
        else
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, pFd, nullptr);
        }

        mReadCallbackMap.erase(pFd);

        if (-1 == res)
        {
            return false;
        }

        return true;
    }

    bool removeWriteHandler(int pFd)
    {
        std::unique_lock<std::mutex> lgCallback(mCallbackMapMutex);

        bool has_read_handler  = mReadCallbackMap.count(pFd);
        bool has_write_handler = mWriteCallbackMap.count(pFd);

        if (!has_write_handler)
        {
            return false;
        }

        int res;
        epoll_event event{};
        event.data.fd = pFd;
        event.events = (EPOLLIN | EPOLLRDHUP);

        if (has_read_handler)
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_MOD, pFd, &event);
        }
        else
        {
            res = epoll_ctl(mEpollFd, EPOLL_CTL_DEL, pFd, nullptr);
        }

        mWriteCallbackMap.erase(pFd);

        if (-1 == res)
        {
            return false;
        }

        return true;
    }

    void run()
    {
        mRunning = true;
        while (mRunning)
        {

            auto nfds = epoll_wait(mEpollFd, mEventCache.data(), mEventCache.size(), -1);
            if (-1 == nfds)
            {
                if (EINTR != errno)
                {
                    throw std::runtime_error(strerror(errno));
                }
                continue;
            }

            for (int i=0; i<nfds; i++)
            {
                std::unique_lock<std::mutex> lg(mCallbackMapMutex);
                auto& event = mEventCache[i];

                if (event.events & (EPOLLOUT))
                {
                    auto foundIt = mWriteCallbackMap.find(event.data.fd);
                    if (mWriteCallbackMap.end() != foundIt)
                    {
                        auto fn = foundIt->second;
                        lg.unlock();
                        fn();
                        lg.lock();
                    }
                }
                if (event.events & (EPOLLIN))
                {
                    auto foundIt = mReadCallbackMap.find(event.data.fd);
                    if (mReadCallbackMap.end() != foundIt)
                    {
                        auto fn = foundIt->second;
                        lg.unlock();
                        fn();
                        lg.lock();
                    }
                }
            }
        }
    }

    void stop()
    {
        mRunning = false;
        notifyEpoll();
    }

private:
    void notifyEpoll()
    {
        uint64_t one = 1;
        static_assert(8 == sizeof(one));
        write(mEventFd, &one, 8);
    }

    size_t mEventCacheSize = 64;
    std::vector<epoll_event> mEventCache;
    std::unordered_map<int, Callback> mWriteCallbackMap;
    std::unordered_map<int, Callback> mReadCallbackMap;
    std::mutex mCallbackMapMutex;

    int mEpollFd;
    int mEventFd;
    bool mRunning;
};

} // namespace bfc

#endif // __EpollReactor_HPP__