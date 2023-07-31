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

#ifndef __WINJECTUM_DUALWIFI_HPP__
#define __WINJECTUM_DUALWIFI_HPP__

#include <sstream>
#include "wifi.hpp"
#include "802_11_filters.hpp"
#include "IWIFI.hpp"

class DualWIFI : public IWIFI
{
public:
    DualWIFI() = delete;
    DualWIFI(std::string tx, std::string rx)
        : tx(tx)
        , rx(rx)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000*100;
        setsockopt(this->rx, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    }

    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sendto(tx, buff, sz, 0, (sockaddr *) &tx.address(), sizeof(struct sockaddr_ll));
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        return ::recv(rx, buffer, sz, 0);
    }

    int handle()
    {
        return rx;
    }

private:
    winject::wifi tx;
    winject::wifi rx;
};

#endif // __WINJECTUM_DUALWIFI_HPP__
