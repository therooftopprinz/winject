#ifndef __WINJECT_TCPSERVER_ENDPOOINT_HPP__
#define __WINJECT_TCPSERVER_ENDPOOINT_HPP__

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

class TCPServerEndPoint : public IEndPoint
{
public:
    TCPServerEndPoint(
        const IEndPoint::config_t& config,
        IRRC& rrc,
        IPDCP& pdcp)
        : config(config)
        , rrc(rrc)
        , pdcp(pdcp)
    {
        int one=1;
        if (0 > serverSock.setsockopt(SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)))
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | ReUseAddr error(#)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPServerEndPoint: failed");
        }

        if (0 > serverSock.bind(bfc::toIp4Port(config.address1)))
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | Bind error(#)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPServerEndPoint: failed");
        }

        ep_event_fd = eventfd(0, EFD_SEMAPHORE);
        if (0 > ep_event_fd)
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | Event FD error(#)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPServerEndPoint: failed");
        }

        if (0 > listen(serverSock.handle(), 1))
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | Listen error(#)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("TCPServerEndPoint: failed");
        }

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
            Logless(*main_logger, TEP_INF,
                "INF | TCPEP# | set_tx_enabled old=# new=#",
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
            Logless(*main_logger, TEP_INF,
                "INF | TCPEP# | set_rx_enabled old=# new=#",
                (int) config.lcid,
                (int) old_rx_enabled,
                (int) is_rx_enabled);
        }
        check_state();
    }

    ~TCPServerEndPoint()
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

        close(ep_event_fd);
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
        if (is_rlf()
            && clientSock)
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | TCPEP# | state RLF!",
                (int)config.lcid);
            
            clientSock = bfc::TcpSocket(-1);
        }

        notify_event(EVENTFD_BREAK_SELECT);
    }

    void on_new_connection()
    {
        Logless(*main_logger, TEP_ERR,
            "ERR | TCPEP# | on_new_connection",
            (int)config.lcid);

        std::unique_lock lg(txrx_mutex);
        auto newSock = accept(serverSock.handle(), nullptr, nullptr);

        Logless(*main_logger, TEP_ERR,
            "ERR | TCPEP# | accepted clientSock=#",
            (int)config.lcid,
            newSock);

        if (clientSock)
        {
            close(newSock);
            return;
        }

        clientSock = bfc::TcpSocket(newSock);

        if (!is_tx_enabled)
        {
            rrc.on_init(config.lcid);
        }
    }

    void run_pdcp_tx()
    {
        pdcp_tx_running = true;
        fd_set recv_set;
        FD_ZERO(&recv_set);
        auto fdmax0 = std::max(serverSock.handle(), ep_event_fd);
    
        while (pdcp_tx_running)
        {
            FD_SET(serverSock.handle(), &recv_set);
            FD_SET(ep_event_fd, &recv_set);

            if (is_active() && clientSock)
            {
                FD_SET(clientSock.handle(), &recv_set);
            }

            auto fdmax1 = std::max(clientSock.handle(), fdmax0);
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

            if (FD_ISSET(clientSock.handle(), &recv_set))
            {
                bfc::BufferView bv{buffer_read, sizeof(buffer_read)};
                auto rv = clientSock.recv(bv);

                if (rv>0)
                {
                    buffer_t b(rv);
                    std::memcpy(b.data(), bv.data(), rv);
                    while (pdcp_tx_running & !pdcp.to_tx(std::move(b)));
                }
                else
                {
                    Logless(*main_logger, TEP_ERR,
                        "ERR | TCPEP# | RLF: client closed error=#",
                        (int)config.lcid,
                        strerror(errno));
                    clientSock = bfc::TcpSocket(-1);
                    rrc.on_rlf(config.lcid);
                }
            }

            if (FD_ISSET(serverSock.handle(), &recv_set))
            {
                on_new_connection();
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

                Logless(*main_logger, TEP_ERR,
                    "INF | TCPEP# | recv event=# nevent=#",
                    (int)config.lcid,
                    val,
                    nevent);

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
        while (pdcp_rx_running)
        {
            auto b = pdcp.to_rx(1000*100);
            if (b.size() && is_active() && clientSock)
            {
                clientSock.send(bfc::BufferView((uint8_t*)b.data(), b.size()));
            }
        }
    }

    config_t config;
    bfc::TcpSocket serverSock;
    bfc::TcpSocket clientSock = bfc::TcpSocket(-1);
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
};

#endif // __WINJECT_TCPSERVER_ENDPOOINT_HPP__
