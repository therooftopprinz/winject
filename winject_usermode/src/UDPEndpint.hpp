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
    struct config_t
    {
        int lcid;
        std::string to_host;
        int to_port;
        std::string from_host;
        int from_port;
    };

    UDPEndPoint(const config_t& config, bfc::IReactor& reactor,
        IPDCP& pdcp)
        : config(config)
        , reactor(reactor)
        , pdcp(pdcp)
    {
        if (0 > sock.bind(bfc::toIp4Port(config.from_host, config.from_port)))
        {
            Logless(*main_logger, Logger::DEBUG,
                "DBG | UDPEndPoint | Bind error(_)", strerror(errno));
            throw std::runtime_error("UDPEndPoint: failed");
        }

        target_addr = bfc::toIp4Port(config.to_host, config.to_port);

        reactor.addReadHandler(sock.handle(), [this](){
                on_sock_read();
            });

        udp_sender_thread = std::thread([this](){run_udp_sender();});
    }

    ~UDPEndPoint()
    {
        udp_sender_running = false;

        if (udp_sender_thread.joinable())
        {
            udp_sender_thread.join();
        }

        reactor.removeReadHandler(sock.handle());
        reactor.removeWriteHandler(sock.handle());
    }

private:
    void run_udp_sender()
    {
        udp_sender_running = true;
        while (udp_sender_running)
        {
            auto b = pdcp.to_rx(1000*1000);
            if (b.size())
            {
                int pFd = sock.handle();
                reactor.addWriteHandler(pFd, [pFd, this, b = std::move(b)]()
                    {
                        sock.sendto(bfc::BufferView((uint8_t*)b.data(), b.size()), target_addr);
                        reactor.removeWriteHandler(pFd);
                    });
            }
        }
    }

    void on_sock_read()
    {
        bfc::BufferView bv{buffer_read, sizeof(buffer_read)};
        bfc::Ip4Port sender_addr;
        auto rv = sock.recvfrom(bv, sender_addr);
        if (rv>0)
        {
            buffer_t b(bv.size());
            std::memcpy(b.data(), bv.data(), bv.size());
            pdcp.to_tx(std::move(b));
        }
    }

    config_t config;
    bfc::IReactor& reactor;
    bfc::UdpSocket sock;
    bfc::Ip4Port target_addr;
    IPDCP& pdcp;

    std::thread udp_sender_thread;
    std::atomic_bool udp_sender_running;

    uint8_t buffer_read[1024*16];
};

#endif // __WINJECTUM_UDPENDPOINT_HPP__
