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

#ifndef __WINJECTUM_ILLC_HPP__
#define __WINJECTUM_ILLC_HPP__

#include <atomic>
#include <bfc/Metric.hpp>

#include "pdu.hpp"
#include "frame_defs.hpp"
#include "info_defs.hpp"

struct ILLC
{
    enum crc_type_e
    {
        E_CRC_TYPE_NONE,
        E_CRC_TYPE_CRC32_04C11DB7
    };

    enum tx_mode_e
    {
        E_TX_MODE_TM,
        E_TX_MODE_AM
    };

    struct tx_config_t
    {
        bool allow_rlf = false;
        tx_mode_e mode = E_TX_MODE_TM;
        size_t arq_window_size = 0;
        size_t max_retx_count = 0;
        crc_type_e crc_type = E_CRC_TYPE_NONE;
    };

    struct rx_config_t
    {
        tx_mode_e mode = E_TX_MODE_TM;
        crc_type_e crc_type = E_CRC_TYPE_NONE;
        bool auto_init_on_rx = false;
    };

    struct stats_t
    {
        bfc::IMetric* pkt_sent = nullptr;
        bfc::IMetric* pkt_resent = nullptr;
        bfc::IMetric* pkt_recv = nullptr;
        bfc::IMetric* bytes_sent = nullptr;
        bfc::IMetric* bytes_resent = nullptr;
        bfc::IMetric* bytes_recv = nullptr;
        bfc::IMetric* tx_enabled = nullptr;
        bfc::IMetric* rx_enabled = nullptr;
    };

    virtual ~ILLC(){}

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;

    virtual void set_tx_enabled(bool) = 0;
    virtual void set_rx_enabled(bool) = 0;
    virtual tx_config_t get_tx_confg() = 0;
    virtual rx_config_t get_rx_confg() = 0;
    virtual void reconfigure(const tx_config_t&) = 0;
    virtual void reconfigure(const rx_config_t&) = 0;
    virtual lcid_t get_lcid() = 0;
    virtual const stats_t& get_stats() = 0;
};

#endif // __WINJECTUM_ILLC_HPP__
