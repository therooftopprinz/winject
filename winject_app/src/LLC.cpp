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


#include <arpa/inet.h>

#include "LLC.hpp"
#include "crc.hpp"

LLC::LLC(
    IPDCP& pdcp,
    IRRC& rrc,
    uint8_t lcid,
    tx_config_t tx_config,
    rx_config_t rx_config)
    : pdcp(pdcp)
    , rrc(rrc)
    , lcid(lcid)
    , tx_ring(1024)
    , sn_to_tx_ring(llc_sn_size, -1)
{
    stats.pkt_sent      = &main_monitor->getMetric(to_llc_stat("pkt_sent", lcid));
    stats.pkt_resent    = &main_monitor->getMetric(to_llc_stat("pkt_resent", lcid));
    stats.pkt_recv      = &main_monitor->getMetric(to_llc_stat("pkt_recv", lcid));
    stats.bytes_sent    = &main_monitor->getMetric(to_llc_stat("bytes_sent", lcid));
    stats.bytes_resent  = &main_monitor->getMetric(to_llc_stat("bytes_resent", lcid));
    stats.bytes_recv    = &main_monitor->getMetric(to_llc_stat("bytes_recv", lcid));
    stats.tx_enabled    = &main_monitor->getMetric(to_llc_stat("tx_enabled", lcid));
    stats.rx_enabled    = &main_monitor->getMetric(to_llc_stat("rx_enabled", lcid));

    reset_stats();

    reconfigure(tx_config);
    reconfigure(rx_config);
}

void LLC::configure_crc_size()
{
    if (tx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
    {
        tx_crc_size = 4;
    }

    if (rx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
    {
        rx_crc_size = 4;
    }
}

lcid_t LLC::get_lcid()
{
    return lcid;
}

void LLC::set_tx_enabled(bool value)
{
    reset_stats();
    stats.tx_enabled->store(value);

    std::unique_lock<std::mutex> lg(tx_mutex);
    auto old_tx_enabled = is_tx_enabled;
    is_tx_enabled = value;

    if (tx_config.mode == E_TX_MODE_AM)
    {
        std::unique_lock<std::mutex> lg(tx_ring_mutex);
        for (auto& tx_ring_elem : tx_ring)
        {
            if(tx_ring_elem.pdcp_pdu.empty())
            {
                tx_ring_elem.pdcp_pdu = allocate_tx_buffer();
            }
            tx_ring_elem.acknowledged = true;
            tx_ring_elem.sent_index = -1;
        }
    }

    {
        for (auto& i : to_retx_list)
        {
            if (i.pdcp_pdu.size())
            {
                free_tx_buffer(std::move(i.pdcp_pdu));
            }
        }
        to_retx_list.clear();
    }

    {
        std::unique_lock lg(to_ack_list_mutex);
        to_ack_list.clear();
    }

    Logless(*main_logger, LLC_INF,
        "INF | LLCT#  | set_tx_enabled old=# new=#",
        (int) lcid,
        (int) old_tx_enabled,
        (int) is_tx_enabled);
}

void LLC::set_rx_enabled(bool value)
{
    reset_stats();
    stats.rx_enabled->store(value);

    std::unique_lock<std::mutex> lg(rx_mutex);
    auto old_rx_enabled = is_rx_enabled;
    is_rx_enabled = value;

    Logless(*main_logger, LLC_INF,
        "INF | LLCR#  | set_rx_enabled old=# new=#",
        (int) lcid,
        (int) old_rx_enabled,
        (int) is_rx_enabled);
}

ILLC::tx_config_t LLC::get_tx_confg()
{
    std::unique_lock<std::mutex> lg(tx_mutex);
    return tx_config;
}

void LLC::reconfigure(const tx_config_t& config)
{
    bool status;
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        tx_config = config;
        status = is_tx_enabled;
        configure_crc_size();

        Logless(*main_logger, LLC_INF, "INF | LLCT#  | reconfigure tx:", (int)lcid);
        Logless(*main_logger, LLC_INF, "INF | LLCT#  |   mode: #", (int)lcid, (int)tx_config.mode);
        Logless(*main_logger, LLC_INF, "INF | LLCT#  |   arq_window_size: #", (int)lcid, tx_config.arq_window_size);
        Logless(*main_logger, LLC_INF, "INF | LLCT#  |   max_retx_count: #", (int)lcid, tx_config.max_retx_count);
        Logless(*main_logger, LLC_INF, "INF | LLCT#  |   crc_type: #", (int)lcid, (int)tx_config.crc_type);
        Logless(*main_logger, LLC_INF, "INF | LLCT#  |   crc_size: #", (int)lcid, tx_crc_size);
    }
    set_tx_enabled(status);
}

ILLC::rx_config_t LLC::get_rx_confg()
{
    std::unique_lock<std::mutex> lg(rx_mutex);
    return rx_config;
}

void LLC::reconfigure(const rx_config_t& config)
{
    bool status;
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        auto old_rx_config = rx_config;
        rx_config = config;
        rx_config.auto_init_on_rx = old_rx_config.auto_init_on_rx;

        status = is_rx_enabled;

        configure_crc_size();

        Logless(*main_logger, LLC_INF, "INF | LLCR#  | reconfigure rx:", (int) lcid);
        Logless(*main_logger, LLC_INF, "INF | LLCR#  |   mode: #", (int) lcid, (int) rx_config.mode);
        Logless(*main_logger, LLC_INF, "INF | LLCR#  |   crc_type: #", (int) lcid, (int) rx_config.crc_type);
        Logless(*main_logger, LLC_INF, "INF | LLCR#  |   crc_size: #", (int) lcid, rx_crc_size);
    }
    set_rx_enabled(status);
}

void LLC::on_tx(tx_info_t& info)
{
    std::unique_lock<std::mutex> tx_lg(tx_mutex);

    if (!is_tx_enabled)
    {
        return;
    }

    auto slot_number = info.in_frame_info.slot_number;

    check_retransmit(slot_number);

    llc_t llc(info.out_pdu.base, info.out_pdu.size);
    llc.crc_size = tx_crc_size;

    // tx_info for slot synchronization and data detection
    if (info.out_pdu.size <= llc.get_header_size())
    {
        info.out_pdu.size = 0;
        // No allocation just inform PDCP for slot update to identify pending tx
        pdcp.on_tx(info);
        info.out_tx_available = info.out_tx_available || to_retx_list.size();
        return;
    }

    info.out_pdu.base = llc.payload();
    info.out_pdu.size -= llc.get_header_size();

    // @note Allocate LLC-DATA-ACKs first
    {
        std::unique_lock<std::mutex> lg(to_ack_list_mutex);
        llc_payload_ack_t* acks = (llc_payload_ack_t*) llc.payload();

        if (to_ack_list.size())
        {
            int n_acks = 0;
            lg.unlock();
            while (info.out_pdu.size >= sizeof(llc_payload_ack_t))
            {
                lg.lock();
                if (!to_ack_list.size())
                {
                    break;
                }

                auto ack = to_ack_list.front();
                to_ack_list.pop_front();
                lg.unlock();

                acks[n_acks].sn = ack.first;
                acks[n_acks].count = ack.second;
                info.out_pdu.size -= sizeof(llc_payload_ack_t);

                Logless(*main_logger, LLC_TRC, "TRC | LLCT#  | ack sn=# cx=#",
                    int(lcid), ack.first, ack.second);

                n_acks++;
            }

            llc.set_payload_size(n_acks*sizeof(llc_payload_ack_t));
            llc.set_SN(0);
            llc.set_A(true);
            llc.set_LCID(lcid);

            size_t crc = 0;
            if (tx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
            {
                auto& crc_actual = *(uint32_t*)(llc.CRC());
                crc_actual= 0;
                crc = crc32_04C11DB7()(llc.base, llc.get_SIZE());
                crc_actual = htonl(crc);
            }

            Logless(*main_logger, LLC_TRC,
                "TRC | LLCT#  | nd-pdu slot=# n_acks=# crc=#",
                (int) lcid,
                slot_number,
                n_acks,
                crc);

            info.out_allocated += llc.get_SIZE();
            llc.base = llc.base + llc.get_SIZE();
        }
    }

    // No allocation just inform PDCP for slot update;
    if (info.out_pdu.size < llc.get_header_size() || !info.in_allow_data)
    {
        info.out_pdu.size = 0;
        // No allocation just inform PDCP for slot update to identify pending tx
        pdcp.on_tx(info);
        info.out_tx_available = info.out_tx_available || to_retx_list.size();
        return;
    }

    llc.set_LCID(lcid);
    llc.set_A(false);
    info.out_pdu.base = llc.payload();
    info.out_pdu.size -= llc.get_header_size();

    // @note Check for retransmit
    bool has_retransmit = false;
    if (to_retx_list.size())
    {
        has_retransmit = true;
        auto& retx_pdu = to_retx_list.front();
        // @note Allocation not enough for retransmit
        if (retx_pdu.pdcp_pdu_size > info.out_pdu.size)
        {
            info.out_pdu.size = 0;
            // @note No allocation just inform PDCP for slot update;
            pdcp.on_tx(info);
            info.out_tx_available = true;
            return;
        }
    }

    size_t init_alloc = info.out_allocated;
    size_t retx_count = 0;
    size_t retx_llc_tx_id = 0;
    // @note Get PDCP from PDCP layer
    if (!has_retransmit)
    {
        pdcp.on_tx(info);
        size_t pdcp_allocation_size = info.out_allocated - init_alloc;
        llc.set_payload_size(pdcp_allocation_size);
        info.out_has_data_loaded = true;
    }
    // @note Get PDCP from retransmit
    else
    {
        auto& retx_pdu = to_retx_list.front();
        retx_count = retx_pdu.retry_count + 1;
        retx_llc_tx_id = retx_pdu.llc_tx_id;

        Logless(*main_logger, LLC_TRC,
            "TRC | LLCT#  | retx=#/# to_retx_list_sz=#",
            int(lcid),
            retx_count,
            tx_config.max_retx_count,
            to_retx_list.size());

        if (retx_count > tx_config.max_retx_count)
        {
            to_retx_list.pop_front();
            if (tx_config.allow_rlf)
            {
                LoglessF(*main_logger, LLC_ERR,
                    "ERR | LLCT#  | RLF triggered",
                    int(lcid));

                info.out_allocated = 0;
                rrc.on_rlf(lcid);
                return;
            }

            LoglessF(*main_logger, LLC_ERR,
                "ERR | LLCT#  | RLF inhibited llc_tx_id=#",
                int(lcid),
                retx_llc_tx_id);

            // RETX dropped, aquire new PDCP PDU

            pdcp.on_tx(info);
            size_t pdcp_allocation_size = info.out_allocated - init_alloc;
            llc.set_payload_size(pdcp_allocation_size);
            info.out_has_data_loaded = true;
        }
        else
        {
            std::memcpy(llc.payload(), retx_pdu.pdcp_pdu.data(), retx_pdu.pdcp_pdu_size);
            free_tx_buffer(std::move(retx_pdu.pdcp_pdu));
            llc.set_payload_size(retx_pdu.pdcp_pdu_size);
            info.out_allocated += retx_pdu.pdcp_pdu_size;
            to_retx_list.pop_front();

            info.out_pdu.size = 0;
            pdcp.on_tx(info);
            info.out_has_data_loaded = true;
        }
    }

    // @note No PDCP allocated
    if (init_alloc == info.out_allocated)
    {
        return;
    }

    info.out_allocated += llc.get_header_size();

    llc.set_SN(sn_counter);
    if (tx_crc_size)
    {
        // @todo : Calculate CRC for this LLC
    }
    increment_sn();

    size_t crc = 0;
    if (tx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
    {
        auto& crc_actual = *(winject::BEU32UA*)(llc.CRC());
        crc_actual= 0;
        crc_actual = crc32_04C11DB7()(llc.base, llc.get_SIZE());
        crc = crc_actual;
    }

    std::unique_lock<std::mutex> tx_ring_lg(tx_ring_mutex);

    // @note Setup tx_ring element index for retransmit check
    auto ack_ck_idx = tx_ring_index(slot_number + tx_config.arq_window_size);
    auto tx_idx = tx_ring_index(slot_number);
    auto& ack_elem = tx_ring[ack_ck_idx];
    auto& tx_elem = tx_ring[tx_idx];

    if (E_TX_MODE_AM == tx_config.mode)
    {
        if (!has_retransmit)
        {
            tx_elem.llc_tx_id = llc_tx_id++;
        }
        else
        {
            tx_elem.llc_tx_id = retx_llc_tx_id;
        }

        tx_elem.retry_count = retx_count;
        tx_elem.acknowledged = false;
        ack_elem.sent_index = tx_idx;
        std::memcpy(tx_elem.pdcp_pdu.data(), llc.payload(), llc.get_payload_size());
        // @note copy PDCP to tx_ring elem needed when retransmitting
        tx_elem.pdcp_pdu_size = llc.get_payload_size();
        sn_to_tx_ring[llc.get_SN()] = ack_ck_idx;
    }
    else
    {
        tx_elem.pdcp_pdu_size = llc.get_payload_size();
        stats.pkt_sent->fetch_add(1);
        stats.bytes_sent->fetch_add(tx_elem.pdcp_pdu_size);
    }

    tx_ring_lg.unlock();
    tx_lg.unlock();

    Logless(*main_logger, LLC_TRC,
        "TRC | LLCT#  | pdu slot=# llc_tx_id=# tx_idx=# expected_ack_idx=# sn=# pdu_sz=# crc=#",
        (int) lcid,
        slot_number,
        tx_elem.llc_tx_id,
        tx_idx,
        ack_ck_idx,
        (int) llc.get_SN(),
        tx_elem.pdcp_pdu_size,
        crc);
}

void LLC::on_rx(rx_info_t& info)
{
    std::unique_lock<std::mutex> lg(rx_mutex);

    if (!is_rx_enabled && rx_config.auto_init_on_rx)
    {
        rrc.on_init(lcid);
        return;
    }

    llc_t llc(info.in_pdu.base, info.in_pdu.size);
    llc.crc_size = rx_crc_size;

    if (llc.get_SIZE() > info.in_pdu.size)
    {
        LoglessF(*main_logger, LLC_ERR, "ERR | LLCT# | invalid llc_size=#, pdu_size=#",
            int(lcid),
            llc.get_SIZE(),
            info.in_pdu.size);
        return;
    }

    size_t crc_expected = 0;
    size_t crc_actual = 0;

    if (rx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
    {
        auto& crc = *(winject::BEU32UA*)(llc.CRC());
        crc_expected = crc;
        crc = 0;
        crc_actual = crc32_04C11DB7()(llc.base, llc.get_SIZE());
    }

    if (crc_actual != crc_expected)
    {
        LoglessF(*main_logger, LLC_ERR, "ERR | LLCT# | CRC failed sn=# llc_sz=# expected=# actual=#",
            (int) lcid,
            (int) llc.get_SN(),
            llc.get_SIZE(),
            crc_expected,
            crc_actual);
        return;
    }

    // @note Handle ack
    if (llc.get_A())
    {
        llc_payload_ack_t* acks = (llc_payload_ack_t*) llc.payload();
        size_t acks_n = llc.get_payload_size()/sizeof(llc_payload_ack_t);
        size_t llc_header_size = llc.get_header_size();
        size_t llc_size = llc.get_SIZE();

        for (size_t i=0; i < acks_n; i++)
        {
            if (llc_header_size+i*sizeof(llc_payload_ack_t) > llc_size)
            {
                LoglessF(*main_logger, LLC_ERR, "ERR | LLCT#  | ack overrun, llc_size=# ack_idx=#",
                    int(lcid), llc_size, i);
                break;
            }

            auto& ack = acks[i];
            llc_sn_t sn = ack.sn & llc_sn_mask;
            for (size_t i=0; i<ack.count; i++)
            {
                std::unique_lock<std::mutex> lg(tx_ring_mutex);
                size_t idx_ = sn_to_tx_ring[sn];
                auto idx = tx_ring_index(idx_);
                auto& ack_slot = tx_ring[idx];
                auto sent_idx = tx_ring_index(ack_slot.sent_index);
                auto& sent_slot = tx_ring[sent_idx];
                sent_slot.acknowledged = true;

                stats.pkt_sent->fetch_add(1);
                stats.bytes_sent->fetch_add(sent_slot.pdcp_pdu_size);

                Logless(*main_logger, LLC_TRC,
                    "TRC | LLCT#  | acked idx=# sn=#",
                    (int) lcid, ack_slot.sent_index,
                    (int) sn);

                sn = llc_sn_mask & (sn+1);
            }
        }
    }
    // @note Handle data
    else
    {
        if (rx_config.mode == ILLC::E_TX_MODE_AM)
        {
            to_acknowledge(llc.get_SN());
        }

        info.in_pdu.base += llc.get_header_size();
        info.in_pdu.size -= llc.get_header_size();

        Logless(*main_logger, LLC_TRC, "TRC | LLCR#  | llc_rx sn=# pdu_sz=# crc=#",
            (int) lcid,
            (int) llc.get_SN(),
            info.in_pdu.size,
            crc_actual);

        stats.bytes_recv->fetch_add(info.in_pdu.size);
        stats.pkt_recv->fetch_add(1);
        pdcp.on_rx(info);
    }
}

void LLC::check_retransmit(size_t slot_number)
{
    if (tx_config.mode != E_TX_MODE_AM)
    {
        return;
    }

    auto& this_slot = tx_ring[tx_ring_index(slot_number)];
    auto& sent_slot = tx_ring[tx_ring_index(this_slot.sent_index)];
    if (sent_slot.acknowledged)
    {
        return;
    }

    Logless(*main_logger, LLC_TRC,
        "TRC | LLCT#  | retxing llc_tx_id=# current_slot=# sent_index=#",
        int(lcid),
        sent_slot.llc_tx_id,
        slot_number,
        this_slot.sent_index);

    // Reset state to default
    sent_slot.acknowledged = true;
    // Queue back retx PDCP
    to_retx_list.emplace_back();
    auto& to_retx = to_retx_list.back();
    to_retx.llc_tx_id = sent_slot.llc_tx_id;
    to_retx.retry_count = sent_slot.retry_count;
    to_retx.pdcp_pdu_size = sent_slot.pdcp_pdu_size;
    to_retx.pdcp_pdu = allocate_tx_buffer();
    sent_slot.pdcp_pdu.swap(to_retx.pdcp_pdu);
    stats.pkt_resent->fetch_add(1);
    stats.bytes_resent->fetch_add(sent_slot.pdcp_pdu_size);
}

void LLC::to_acknowledge(llc_sn_t sn)
{
    std::unique_lock<std::mutex> lg(to_ack_list_mutex);
    if (!to_ack_list.size())
    {
        to_ack_list.emplace_back(sn, 1);
    }
    else
    {
        auto& first = *to_ack_list.begin();
        if (first.first+first.second == sn)
        {
            first.second++;
        }
        else
        {
            to_ack_list.emplace_back(sn, 1);
        }
    }
}

void LLC::increment_sn()
{
    sn_counter = (sn_counter+1) & llc_sn_mask;
}

size_t LLC::tx_ring_index(size_t slot)
{
    return slot % tx_ring.size();
}

buffer_t LLC::allocate_tx_buffer()
{
    if (tx_buffer_pool.size())
    {
        auto rv = std::move(tx_buffer_pool.back());
        tx_buffer_pool.pop_back();
        return std::move(rv);
    }

    return buffer_t(llc_max_size);
}

void LLC::free_tx_buffer(buffer_t&& buffer)
{
    tx_buffer_pool.emplace_back(std::move(buffer));
}

const LLC::stats_t& LLC::get_stats()
{
    return stats;
}

void LLC::reset_stats()
{
    stats.pkt_sent->store(0);
    stats.pkt_resent->store(0);
    stats.pkt_recv->store(0);
    stats.bytes_sent->store(0);
    stats.bytes_resent->store(0);
    stats.bytes_recv->store(0);
}