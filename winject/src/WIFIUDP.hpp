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
            LoglessF(*main_logger, TEP_ERR, "ERR | WIFIUDP | Bind error(_) can't setup WIFIUDP", strerror(errno));
            throw std::runtime_error("Can't setup WIFIUDP");
        }
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
