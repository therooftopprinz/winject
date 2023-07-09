#ifndef __WINJECTUM_WIFIUDP_HPP__
#define __WINJECTUM_WIFIUDP_HPP__

#include <sstream>
#include <bfc/Udp.hpp>
#include "IWIFI.hpp"
#include "Logger.hpp"

class WIFIUDP : public IWIFI
{
public:
    WIFIUDP() = delete;
    WIFIUDP(std::string tx_address, std::string rx_address)
        : tx_address(bfc::toIp4Port(tx_address))
        , rx_address(bfc::toIp4Port(rx_address))
    {
        if (sock.bind(this->rx_address) < 0)
        {
            Logless(*main_logger, TEP_ERR, "ERR | WIFIUDP | Bind error(_) can't setup WIFIUDP", strerror(errno));
            throw std::runtime_error("Can't setup WIFIUDP");
        }

        // struct timeval tv;
        // tv.tv_sec = 0;
        // tv.tv_usec = 1000*100;
        // setsockopt(sock.handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }
    
    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sock.sendto(bfc::ConstBufferView(buff, sz+4), tx_address);
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        bfc::Ip4Port from;
        bfc::BufferView bv(buffer, sz);
        return sock.recvfrom(bv, from);
    }

    int handle()
    {
        return sock.handle();
    }

private:
    bfc::Ip4Port tx_address;
    bfc::Ip4Port rx_address;
    bfc::UdpSocket sock;

};

#endif // __WINJECTUM_WIFIUDP_HPP__
