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

#ifndef __WINJECTUM_IENDPOINT_HPP__
#define __WINJECTUM_IENDPOINT_HPP__

#include <bfc/IMetric.hpp>

struct IEndPoint
{
    struct config_t
    {
        lcid_t lcid;
        std::string type;
        std::string address1;
        std::string address2;
    };

    struct stats_t
    {
        bfc::IMetric* tx_enabled = nullptr;
        bfc::IMetric* rx_enabled = nullptr;
    };

    virtual void set_tx_enabled(bool) = 0;
    virtual void set_rx_enabled(bool) = 0;

    virtual ~IEndPoint() {}
};

#endif // __WINJECTUM_IENDPOINT_HPP__
