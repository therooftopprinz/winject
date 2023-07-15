#ifndef __WINJECT_TCPCLIENT_ENDPOOINT_HPP__
#define __WINJECT_TCPCLIENT_ENDPOOINT_HPP__

#include <string>
#include <atomic>
#include <mutex>

#include <sys/eventfd.h>

#include <bfc/IReactor.hpp>
#include <bfc/Tcp.hpp>

#include "IPDCP.hpp"
#include "IRRC.hpp"
#include "IEndPoint.hpp"
#include "Logger.hpp"

class TCPClientEndPoint : public IEndPoint
{
public:
    TCPClientEndPoint(
        const IEndPoint::config_t& config,
        IRRC& rrc,
        IPDCP& pdcp)
        : config(config)
        , rrc(rrc)
        , pdcp(pdcp)
    {
        ep_event_fd = eventfd(0, EFD_SEMAPHORE);
        if (0 > ep_event_fd)
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | Event FD error(_)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPEndPoint: failed");
        }

        target_addr = bfc::toIp4Port(config.address1);

        pdcp_tx_thread = std::thread([this, lcid = config.lcid](){
            std::string name = "PDCP_";
            name += std::to_string(lcid);
            name += "_TX";
            pthread_setname_np(pthread_self(), name.c_str());
            run_pdcp_tx();});

        pdcp_rx_thread = std::thread([this, lcid = config.lcid](){
            std::string name = "PDCP_";
            name += std::to_string(lcid);
            name += "_RX";
            pthread_setname_np(pthread_self(), name.c_str());
            run_pdcp_rx();});
    }

    void set_tx_enabled(bool value) override
    {
        {
            std::unique_lock lg(txrx_mutex);
            auto old_tx_enabled = is_tx_enabled;
            is_tx_enabled = value;
            Logless(*main_logger, LLC_INF,
                "INF | TCPEP#  | set_tx_enabled old=# new=#",
                (int) config.lcid,
                (int) old_tx_enabled,
                (int) is_tx_enabled);
        }

        check_state();
    }

    void set_rx_enabled(bool value) override
    {
        {
            std::unique_lock lg(txrx_mutex);
            auto old_rx_enabled = is_tx_enabled;
            is_rx_enabled = value;
            Logless(*main_logger, LLC_INF,
                "INF | TCPEP#  | set_rx_enabled old=# new=#",
                (int) config.lcid,
                (int) old_rx_enabled,
                (int) is_rx_enabled);
        }
        check_state();
    }

    ~TCPClientEndPoint()
    {
        pdcp_tx_running = false;
        pdcp_rx_running = false;

        notify_event(EVENTFD_STOP);

        if (pdcp_tx_thread.joinable())
        {
            pdcp_tx_thread.join();
        }

        if (pdcp_rx_thread.joinable())
        {
            pdcp_rx_thread.join();
        }
    }

private:
    constexpr static uint64_t EVENTFD_STOP = 0xFFFFFFFFFFFFFFFF;
    constexpr static uint64_t EVENTFD_BREAK_SELECT = 1;

    void notify_event(uint64_t val)
    {
        write(ep_event_fd, &val, sizeof(val));
    }

    bool is_active()
    {
        return is_tx_enabled && is_rx_enabled;
    }

    bool is_rlf()
    {
        return is_tx_enabled == false
            && is_rx_enabled == false;
    }

    void check_state()
    {
        // RLF and client active
        if (is_rlf())
        {
            targetSock = bfc::TcpSocket(-1);
        }
        else if (is_active())
        {
            targetSock = bfc::TcpSocket();
            if (0 > targetSock.connect(target_addr))
            {
                Logless(*main_logger, TEP_ERR,
                    "ERR | TCPEP# | connect error(#)",
                    (int)config.lcid,
                    strerror(errno));
                targetSock = bfc::TcpSocket(-1);
                rrc.on_rlf(pdcp.get_attached_lcid());
            }
        }

        notify_event(EVENTFD_BREAK_SELECT);
    }

    void run_pdcp_tx()
    {
        pdcp_tx_running = true;
        fd_set recv_set;
        FD_ZERO(&recv_set);
    
        while (pdcp_tx_running)
        {
            FD_SET(targetSock.handle(), &recv_set);
            FD_SET(ep_event_fd, &recv_set);

            if (is_active() && targetSock)
            {
                FD_SET(targetSock.handle(), &recv_set);
            }

            auto fdmax1 = std::max(targetSock.handle(), ep_event_fd);
            auto maxfd = fdmax1+1;

            auto rv = select(maxfd, &recv_set, nullptr, nullptr, nullptr);

            if (EINTR == errno)
            {
                continue;
            }

            if ( rv < 0)
            {
                Logless(*main_logger, TEP_ERR,
                    "ERR | TCPEP# | select error(_)",
                    (int)config.lcid,
                    strerror(errno));
                return;
            }

            if (FD_ISSET(targetSock.handle(), &recv_set))
            {
                bfc::BufferView bv{buffer_read, sizeof(buffer_read)};
                auto rv = targetSock.recv(bv);

                if (rv>0)
                {
                    buffer_t b(rv);
                    std::memcpy(b.data(), bv.data(), rv);
                    while (pdcp_tx_running & !pdcp.to_tx(std::move(b)));
                }
                else
                {
                    Logless(*main_logger, TEP_ERR,
                        "ERR | TCPEP# | RLF: target closed error=#",
                        (int)config.lcid,
                        strerror(errno));
                    targetSock = bfc::TcpSocket(-1);
                    rrc.on_rlf(config.lcid);
                }
            }

            if (FD_ISSET(ep_event_fd, &recv_set))
            {
                uint64_t val;
                read(ep_event_fd, &val, sizeof(val));
                if (EVENTFD_STOP == val)
                {
                    Logless(*main_logger, TEP_ERR,
                        "INF | TCPEP# | exiting tx",
                        (int)config.lcid);
                    return;
                }
            }
        }
    }

    void run_pdcp_rx()
    {
        pdcp_rx_running = true;
        buffer_t pending;

        while (pdcp_rx_running)
        {
            if (!pending.size())
            {
                pending = pdcp.to_rx(1000*100);
            }

            if (!pending.size())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (pending.size() && is_active() && targetSock)
            {
                targetSock.send(bfc::BufferView((uint8_t*)pending.data(), pending.size()));
                pending.resize(0);
            }
        }
    }

    config_t config;
    bfc::TcpSocket targetSock = bfc::TcpSocket(-1);
    bfc::Ip4Port target_addr;
    IRRC& rrc;
    IPDCP& pdcp;

    std::thread pdcp_tx_thread;
    std::atomic_bool pdcp_tx_running;

    std::thread pdcp_rx_thread;
    std::atomic_bool pdcp_rx_running;

    uint8_t buffer_read[1024];

    std::mutex txrx_mutex;
    bool is_tx_enabled = false;
    bool is_rx_enabled = false;

    int ep_event_fd;
};

#endif // __WINJECT_TCPCLIENT_ENDPOOINT_HPP__
