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
    UDPEndPoint(const IEndPoint::config_t& config, bfc::IReactor& reactor,
        IPDCP& pdcp)
        : config(config)
        , reactor(reactor)
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

        target_addr = bfc::toIp4Port(config.address2);

        reactor.addReadHandler(sock.handle(), [this](){
                on_sock_read();
            });

        udp_sender_thread = std::thread([this](){run_udp_sender();});
    }

    ~UDPEndPoint()
    {
        if (udp_sender_thread.joinable())
        {
            udp_sender_running = false;
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
                sock.sendto(bfc::BufferView((uint8_t*)b.data(), b.size()), target_addr);
                // reactor.addWriteHandler(pFd, [pFd, this, b = std::move(b)]()
                //     {
                //         sock.sendto(bfc::BufferView((uint8_t*)b.data(), b.size()), target_addr);
                //         reactor.removeWriteHandler(pFd);
                //     });
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
            buffer_t b(rv);
            std::memcpy(b.data(), bv.data(), rv);
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
