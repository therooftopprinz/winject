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

#ifndef __WINJECTUM_WIFIUDP_HPP__
#define __WINJECTUM_WIFIUDP_HPP__

#include <sstream>
#include <bfc/socket.hpp>
#include "IWIFI.hpp"
#include "Logger.hpp"

inline sockaddr_in wifiudp_to_sockaddr4(const std::string& hostport)
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

class WIFIUDP : public IWIFI
{
public:
    WIFIUDP() = delete;
    WIFIUDP(std::string tx_address, std::string rx_address)
        : tx_address(wifiudp_to_sockaddr4(tx_address))
        , rx_address(wifiudp_to_sockaddr4(rx_address))
        , sock(bfc::create_udp4())
    {
        if (sock.bind(this->rx_address) < 0)
        {
            LoglessF(*main_logger, TEP_ERR, "ERR | WIFIUDP | Bind error(_) can't setup WIFIUDP", strerror(errno));
            throw std::runtime_error("Can't setup WIFIUDP");
        }
    }

    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sock.send(
            bfc::const_buffer_view(buff, sz+4),
            0,
            (const sockaddr*)&tx_address,
            sizeof(tx_address));
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        bfc::buffer_view bv(buffer, sz);
        sockaddr_in from;
        socklen_t from_sz = sizeof(from);
        return sock.recv(bv, 0, (sockaddr*)&from, &from_sz);
    }

    int handle()
    {
        return sock.fd();
    }

private:
    sockaddr_in tx_address;
    sockaddr_in rx_address;
    bfc::socket sock;

};

#endif // __WINJECTUM_WIFIUDP_HPP__
