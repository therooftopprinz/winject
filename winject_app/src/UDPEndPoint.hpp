#ifndef __WINJECTUM_UDPENDPOINT_HPP__
#define __WINJECTUM_UDPENDPOINT_HPP__

#include <string>
#include <atomic>

#include <bfc/IReactor.hpp>
#include <bfc/Udp.hpp>

#include "IPDCP.hpp"
#include "IEndPoint.hpp"
#include "Logger.hpp"

class UDPEndPoint : public IEndPoint
{
public:
    UDPEndPoint(
        const IEndPoint::config_t& config,
        IPDCP& pdcp)
        : config(config)
        , pdcp(pdcp)
    {
        if (0 > sock.bind(bfc::toIp4Port(config.address1)))
        {
            Logless(*main_logger, TEP_ERR,
                "ERR | UDPEP# | Bind error(_)",
                (int)config.lcid,
                strerror(errno));
            throw std::runtime_error("UDPEndPoint: failed");
        }

        // struct timeval tv;
        // tv.tv_sec = 0;
        // tv.tv_usec = 1000*100;
        // setsockopt(sock.handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        target_addr = bfc::toIp4Port(config.address2);

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
        std::unique_lock<std::mutex> lg(txrx_mutex);
        is_tx_enabled = value;
    }

    void set_rx_enabled(bool value) override
    {
        std::unique_lock<std::mutex> lg(txrx_mutex);
        is_rx_enabled = value;
    }

    ~UDPEndPoint()
    {
        pdcp_tx_running = false;
        pdcp_rx_running = false;

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
        while (pdcp_tx_running)
        {
            bfc::BufferView bv{buffer_read, sizeof(buffer_read)};
            bfc::Ip4Port sender_addr;
            auto rv = sock.recvfrom(bv, sender_addr);
            if (EAGAIN == errno || EWOULDBLOCK == errno || EINTR == errno)
            {
                continue;
            }
            if (rv>0)
            {
                buffer_t b(rv);
                std::memcpy(b.data(), bv.data(), rv);
                while (pdcp_tx_running & !pdcp.to_tx(std::move(b)));
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
                int pFd = sock.handle();
                sock.sendto(bfc::BufferView((uint8_t*)b.data(), b.size()), target_addr);
            }
        }
    }

    config_t config;
    bfc::UdpSocket sock;
    bfc::Ip4Port target_addr;
    IPDCP& pdcp;

    std::thread pdcp_tx_thread;
    std::atomic_bool pdcp_tx_running;

    std::thread pdcp_rx_thread;
    std::atomic_bool pdcp_rx_running;

    uint8_t buffer_read[1024*16];

    std::mutex txrx_mutex;
    bool is_tx_enabled = false;
    bool is_rx_enabled = false;
};

#endif // __WINJECTUM_UDPENDPOINT_HPP__
