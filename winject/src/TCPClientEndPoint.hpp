/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WINJECT_TCPCLIENT_ENDPOOINT_HPP__
#define __WINJECT_TCPCLIENT_ENDPOOINT_HPP__

#include <string>
#include <deque>
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
        stats.tx_enabled = &main_monitor->getMetric(to_ep_stat("tx_enabled", config.lcid));
        stats.rx_enabled = &main_monitor->getMetric(to_ep_stat("rx_enabled", config.lcid));

        ep_event_fd = eventfd(0, EFD_SEMAPHORE);
        if (0 > ep_event_fd)
        {
            LoglessF(*main_logger, TEP_ERR,
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
        stats.tx_enabled->store(value);
        {
            std::unique_lock lg(txrx_mutex);
            auto old_tx_enabled = is_tx_enabled;
            is_tx_enabled = value;
            Logless(*main_logger, LLC_INF,
                "INF | TCPEP# | set_tx_enabled old=# new=#",
                (int) config.lcid,
                (int) old_tx_enabled,
                (int) is_tx_enabled);
        }

        check_state();
    }

    void set_rx_enabled(bool value) override
    {
        stats.rx_enabled->store(value);
        {
            std::unique_lock lg(txrx_mutex);
            auto old_rx_enabled = is_tx_enabled;
            is_rx_enabled = value;
            Logless(*main_logger, LLC_INF,
                "INF | TCPEP# | set_rx_enabled old=# new=#",
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

    void notify_event(uint64_t event)
    {
        Logless(*main_logger, TEP_ERR,
            "INF | TCPEP# | notify event=#",
            (int)config.lcid,
            event);

        {
            std::unique_lock lg(ep_event_mutex);
            ep_event.push_back(event);
        }

        uint64_t one = 1;
        write(ep_event_fd, &one, sizeof(one));
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
            LoglessF(*main_logger, TEP_ERR,
                "ERR | TCPEP# | RLF detected, target will be disconnected.",
                (int)config.lcid,
                strerror(errno));
            targetSock = bfc::TcpSocket(-1);
        }
        else if (is_active())
        {
            targetSock = bfc::TcpSocket();
            if (targetSock.connect(target_addr) >= 0)
            {
                Logless(*main_logger, TEP_INF,
                    "INF | TCPEP# | connected to targetSock=#",
                    (int)config.lcid,
                    targetSock.handle());
            }
            else
            {
                LoglessF(*main_logger, TEP_ERR,
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
                LoglessF(*main_logger, TEP_ERR,
                    "ERR | TCPEP# | select error(#)",
                    (int)config.lcid,
                    strerror(errno));
                continue;
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
                    LoglessF(*main_logger, TEP_ERR,
                        "ERR | TCPEP# | RLF: target closed error=#",
                        (int)config.lcid,
                        strerror(errno));
                    targetSock = bfc::TcpSocket(-1);
                    rrc.on_rlf(config.lcid);
                }
            }

            if (FD_ISSET(ep_event_fd, &recv_set))
            {
                uint64_t nevent;
                read(ep_event_fd, &nevent, sizeof(nevent));

                uint64_t val;

                {
                    std::unique_lock lg(ep_event_mutex);
                    val = ep_event.front();
                    ep_event.pop_front();
                }

                if (EVENTFD_STOP == val)
                {
                    break;
                }
            }
        }
        Logless(*main_logger, TEP_ERR,
                "INF | TCPEP# | exiting tx",
            (int)config.lcid);
        pdcp_tx_running = false;
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
    std::deque<uint64_t> ep_event;
    std::shared_mutex ep_event_mutex;

    stats_t stats;
};

#endif // __WINJECT_TCPCLIENT_ENDPOOINT_HPP__
