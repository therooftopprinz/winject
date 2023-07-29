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

#ifndef __WINJECTUM_LLC_HPP__
#define __WINJECTUM_LLC_HPP__

#include <memory>
#include <optional>
#include <cstring>
#include <list>
#include <mutex>


#include "ILLC.hpp"
#include "IPDCP.hpp"
#include "IRRC.hpp"
#include "frame_defs.hpp"
#include "Logger.hpp"

class LLC : public ILLC
{
public:
    LLC(
        IPDCP& pdcp,
        IRRC& rrc,
        uint8_t lcid,
        tx_config_t tx_config,
        rx_config_t rx_config);

    lcid_t get_lcid() override;

    void set_tx_enabled(bool value) override;
    void set_rx_enabled(bool value) override;

    tx_config_t get_tx_confg() override;

    void reconfigure(const tx_config_t& config) override;

    rx_config_t get_rx_confg() override;

    virtual void reconfigure(const rx_config_t& config) override;

    void on_tx(tx_info_t& info) override;
    void on_rx(rx_info_t& info) override;

private:
    void configure_crc_size();
    void check_retransmit(size_t slot_number);
    void to_acknowledge(llc_sn_t sn);
    void increment_sn();
    size_t tx_ring_index(size_t slot);
    buffer_t allocate_tx_buffer();
    void free_tx_buffer(buffer_t&& buffer);
    const stats_t& get_stats();
    void reset_stats();

    // @brief Used to indicated retransmit slot
    struct tx_ring_elem_t
    {
        size_t sent_index = -1;
        size_t llc_tx_id = 0;
        size_t retry_count = 0;
        size_t pdcp_pdu_size = 0;
        buffer_t pdcp_pdu;
        bool acknowledged = true;
    };

    struct retx_elem_t
    {
        size_t llc_tx_id = 0;
        size_t retry_count;
        size_t pdcp_pdu_size;
        buffer_t pdcp_pdu;
    };

    IPDCP& pdcp;
    IRRC& rrc;
    // @brief Logical Channel Identifier
    lcid_t lcid;

    size_t llc_tx_id = 0;

    // TX Data Structures ------------------------------------------------------
    std::mutex tx_mutex;
    bool is_tx_enabled = false;
    tx_config_t tx_config;
    size_t tx_crc_size = 0;

    std::list<buffer_t> tx_buffer_pool;
    llc_sn_t  sn_counter = 0;
    std::list<retx_elem_t> to_retx_list;

    // Common Data Structures --------------------------------------------------
    std::vector<tx_ring_elem_t> tx_ring;
    std::vector<size_t> sn_to_tx_ring;
    std::mutex tx_ring_mutex;

    std::list<std::pair<size_t,size_t>> to_ack_list;
    std::mutex to_ack_list_mutex;

    // RX Data Structures ------------------------------------------------------
    std::mutex rx_mutex;
    bool is_rx_enabled = false;
    rx_config_t rx_config;
    size_t rx_crc_size = 0;

    // Statistics --------------------------------------------------------------
    stats_t stats;

    static constexpr size_t llc_max_size = 1500;
};

#endif // __WINJECTUM_LLC_HPP__
