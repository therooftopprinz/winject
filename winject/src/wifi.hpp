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

#ifndef __WINJECT_WIFI_HPP__
#define __WINJECT_WIFI_HPP__

#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>

#include <string>
#include <cstring>

namespace winject
{

class wifi
{
public:
    wifi(const std::string& device)
        : raw_sd(socket(AF_PACKET, SOCK_RAW, htons(ETH_P_802_2)))
    {
        if (raw_sd < 0)
        {
            return;
        }

        memset(&raw_if_idx, 0, sizeof(ifreq));
        strncpy(raw_if_idx.ifr_name, device.c_str(), device.size());
        if (ioctl(raw_sd, SIOCGIFINDEX, &raw_if_idx) < 0)
        {
            state = E_STATE_SIOCGIFINDEX_FAILED;
            return;
        }

        memset(&raw_if_mac, 0, sizeof(struct ifreq));
        strncpy(raw_if_mac.ifr_name, device.c_str(), device.size());
        if (ioctl(raw_sd, SIOCGIFHWADDR, &raw_if_mac) < 0)
        {
            state = E_STATE_SIOCGIFHWADDR_FAILED;
            return;
        }

        memset(&sll, 0, sizeof(sll));
        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = raw_if_idx.ifr_ifindex;
        sll.sll_protocol = htons(ETH_P_802_2);

        if ((bind(raw_sd, (sockaddr*) &sll, sizeof(sll))) < 0)
        {
            state = E_STATE_BIND_FAILED;
            return;
        }
    }

    sockaddr_ll& address()
    {
        return sll;
    }

    enum state_e {
        E_OK,
        E_STATE_SOCKET_FAILED,
        E_STATE_SIOCGIFINDEX_FAILED,
        E_STATE_SIOCGIFHWADDR_FAILED,
        E_STATE_BIND_FAILED,
    };

    operator int()
    {
        return raw_sd;
    }

private:
    state_e state = E_OK;
    int raw_sd = -1;
    ifreq raw_if_idx;
    ifreq raw_if_mac;
    sockaddr_ll sll;

};

} // namespace winject

#endif // __WINJECT_WIFI_HPP__