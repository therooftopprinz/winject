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

#ifndef __WINJECT_UDPENDPOINT_HPP__
#define __WINJECT_UDPENDPOINT_HPP__

#include <string>
#include <atomic>
#include <mutex>

#include <sys/eventfd.h>

#include <sstream>
#include <bfc/socket.hpp>

#include "IPDCP.hpp"
#include "IEndPoint.hpp"
#include "Logger.hpp"

inline sockaddr_in udp_ep_to_sockaddr4(const std::string& hostport)
{
    std::stringstream ss(hostport);
    std::string host;
    std::string port_str;

    if (!std::getline(ss, host, ':') || !std::getline(ss, port_str))
    {
        return {};
    }

    uint16_t port = static_cast<uint16_t>(std::stoul(port_str));
    return bfc::ip4_port_to_sockaddr(host, port);
}

class UDPEndPoint : public IEndPoint
{
public:
    UDPEndPoint(
        const IEndPoint::config_t& config,
        IPDCP& pdcp)
        : config(config)
        , pdcp(pdcp)
        , sock(bfc::create_udp4())
    {
        stats.tx_enabled = &main_monitor->get_metric(to_ep_stat("tx_enabled", config.lcid));
        stats.rx_enabled = &main_monitor->get_metric(to_ep_stat("rx_enabled", config.lcid));

        auto bind_addr = udp_ep_to_sockaddr4(config.address1);
        if (0 > sock.bind(bind_addr))
        {
            LoglessF(*main_logger, TEP_ERR,
                "ERR | UDPEP# | Bind error(_)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("UDPEndPoint: failed");
        }

        ep_event_fd = eventfd(0, 0);
        if (0 > ep_event_fd)
        {
            LoglessF(*main_logger, TEP_ERR,
                "ERR | TCPEP# | Event FD error(_)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPEndPoint: failed");
        }

        target_addr = udp_ep_to_sockaddr4(config.address2);

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
        std::unique_lock lg(txrx_mutex);
        is_tx_enabled = value;
    }

    void set_rx_enabled(bool value) override
    {
        stats.rx_enabled->store(value);
        std::unique_lock lg(txrx_mutex);
        is_rx_enabled = value;
    }

    ~UDPEndPoint()
    {
        pdcp_tx_running = false;
        pdcp_rx_running = false;

        uint64_t one = 1;
        write(ep_event_fd, &one, sizeof(one));

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
    void run_pdcp_tx()
    {
        pdcp_tx_running = true;
        // struct timeval tv = {1, 0};
        fd_set recv_set;
        FD_ZERO(&recv_set);
        auto maxfd = std::max(sock.fd(), ep_event_fd)+1;

        while (pdcp_tx_running)
        {
            FD_SET(sock.fd(), &recv_set);
            FD_SET(ep_event_fd,   &recv_set);

            auto rv = select(maxfd, &recv_set, nullptr, nullptr, nullptr);

            if (EAGAIN == errno || EINTR == errno)
            {
                continue;
            }

            if ( rv < 0)
            {
                LoglessF(*main_logger, TEP_ERR,
                    "ERR | UDPEP# | select error(_)",
                    (int)config.lcid,
                    strerror(errno));
                return;
            }

            if (FD_ISSET(sock.fd(), &recv_set))
            {
                bfc::buffer_view bv{buffer_read, sizeof(buffer_read)};
                sockaddr_in sender_addr;
                socklen_t sender_addr_sz = sizeof(sender_addr);
                auto rv = sock.recv(bv, 0, (sockaddr*)&sender_addr, &sender_addr_sz);

                if (rv>0)
                {
                    if (config.reply_to_last)
                    {
                        target_addr = sender_addr;
                    }
                    buffer_t b(rv);
                    std::memcpy(b.data(), bv.data(), rv);
                    while (pdcp_tx_running & !pdcp.to_tx(std::move(b)));
                }
            }
            if (FD_ISSET(ep_event_fd, &recv_set))
            {
                return;
            }
        }
    }

    void run_pdcp_rx()
    {
        pdcp_rx_running = true;
        while (pdcp_rx_running)
        {
            auto b = pdcp.to_rx(1000*100);
            if (b.size())
            {
                int pFd = sock.fd();
                (void)pFd;
                sock.send(
                    bfc::buffer_view((uint8_t*)b.data(), b.size()),
                    0,
                    (const sockaddr*)&target_addr,
                    sizeof(target_addr));
            }
        }
    }

    config_t config;
    bfc::socket sock;
    sockaddr_in target_addr;
    IPDCP& pdcp;

    std::thread pdcp_tx_thread;
    std::atomic_bool pdcp_tx_running;

    std::thread pdcp_rx_thread;
    std::atomic_bool pdcp_rx_running;

    uint8_t buffer_read[1024*16];

    std::mutex txrx_mutex;
    bool is_tx_enabled = false;
    bool is_rx_enabled = false;

    int ep_event_fd;

    stats_t stats;
};

#endif // __WINJECT_UDPENDPOINT_HPP__
