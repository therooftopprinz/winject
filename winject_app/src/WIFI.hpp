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

#ifndef __WINJECTUM_WIFI_HPP__
#define __WINJECTUM_WIFI_HPP__

#include <sstream>
#include <winject/wifi.hpp>
#include <winject/802_11_filters.hpp>
#include "IWIFI.hpp"

class WIFI : public IWIFI
{
public:
    WIFI() = delete;
    WIFI(std::string device)
        : wdev(device)
    {
    }

    ssize_t send(const uint8_t* buff, size_t sz)
    {
        return sendto(wdev, buff, sz, 0, (sockaddr *) &wdev.address(), sizeof(struct sockaddr_ll));
    }

    ssize_t recv(uint8_t* buffer, size_t sz)
    {
        return ::recv(wdev, buffer, sz, 0);
    }

    int handle()
    {
        return wdev;
    }

private:
    winject::wifi wdev;
};

#endif // __WINJECTUM_WIFI_HPP__
