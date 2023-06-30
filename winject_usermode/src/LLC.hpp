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
        std::shared_ptr<IPDCP> pdcp,
        IRRC& rrc,
        uint8_t lcid,
        tx_config_t tx_config,
        rx_config_t rx_config)
        : pdcp(pdcp)
        , rrc(rrc)
        , lcid(lcid)
        , tx_config(tx_config)
        , rx_config(rx_config)
        , tx_ring(1024)
        , sn_to_tx_ring(llc_sn_size, -1)
    {
    }

    lcid_t get_lcid()
    {
        return lcid;
    }

    void set_tx_enabled(bool value)
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        is_tx_enabled = value;

        if (tx_config.mode == E_TX_MODE_AM)
        {
            for (auto& tx_ring_elem : tx_ring)
            {
                if(tx_ring_elem.pdcp_pdu.empty())
                {
                    tx_ring_elem.pdcp_pdu = allocate_buffer();
                }
                tx_ring_elem.acknowledged = true;
                tx_ring_elem.sent_index = -1;
            }
        }

        for (auto& i : to_retx_list)
        {
            if (i.pdcp_pdu.size())
            {
                free_buffer(std::move(i.pdcp_pdu));
            }
        }

        to_retx_list.clear();

        Logless(*main_logger, LLC_INF,
            "INF | LLC#   | tx_enable=#",
            (int) lcid,
            (int) is_tx_enabled);
    }

    void set_rx_enabled(bool value)
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        is_rx_enabled = value;

        Logless(*main_logger, LLC_INF,
            "INF | LLC#   | rx_enable=#",
            (int) lcid,
            (int)is_rx_enabled);
    }

    virtual tx_config_t get_tx_confg()
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        return tx_config;
    }

    virtual void reconfigure(const tx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::mutex> lg(tx_mutex);
            tx_config = config;
            status = is_tx_enabled;
            if (tx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
            {
                tx_crc_size = 4;
            }

            Logless(*main_logger, LLC_INF, "INF | LLC#   | reconfigure tx:", (int)lcid);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   mode: #", (int)lcid, (int)tx_config.mode);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   arq_window_size: #", (int)lcid, tx_config.arq_window_size);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   max_retx_count: #", (int)lcid, tx_config.max_retx_count);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   crc_type: #", (int)lcid, (int)tx_config.crc_type);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   crc_size: #", (int)lcid, tx_crc_size);
        }
        set_tx_enabled(status);
    }

    virtual rx_config_t get_rx_confg()
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        return rx_config;
    }

    virtual void reconfigure(const rx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::mutex> lg(rx_mutex);
            rx_config = config;
            status = is_rx_enabled;

            if (rx_config.crc_type == E_CRC_TYPE_CRC32_04C11DB7)
            {
                rx_crc_size = 4;
            }

            Logless(*main_logger, LLC_INF, "INF | LLC#   | reconfigure rx:", (int) lcid);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   mode: #", (int) lcid, (int) rx_config.peer_mode);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   crc_type: #", (int) lcid, (int) rx_config.crc_type);
            Logless(*main_logger, LLC_INF, "INF | LLC#   |   crc_size: #", (int) lcid, rx_crc_size);
        }
        set_rx_enabled(status);
    }

    void print_stats()
    {
        LoglessF(*main_logger, LLC_STS, "STS | LLC#   | RAW #,#,#,#,#,#", (int) lcid,
            stats.bytes_recv.load(),
            stats.bytes_sent.load(),
            stats.bytes_resent.load(),
            stats.pkt_recv.load(),
            stats.pkt_sent.load(),
            stats.pkt_resent.load());

        Logless(*main_logger, LLC_STS, "STS | LLC#   | STATS:", (int) lcid);
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   bytes_recv:    #", (int) lcid, stats.bytes_recv.load());
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   bytes_sent:    #", (int) lcid, stats.bytes_sent.load());
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   bytes_resent:  #", (int) lcid, stats.bytes_resent.load());
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   pkt_recv:      #", (int) lcid, stats.pkt_recv.load());
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   pkt_sent:      #", (int) lcid, stats.pkt_sent.load());
        Logless(*main_logger, LLC_STS, "STS | LLC#   |   pkt_resent:    #", (int) lcid, stats.pkt_resent.load());
    }

    /**
     * @brief on_tx
     *   Called by scheduler to fillup buffer with LLC PDUs
     *   Should not be called again once tx_info_t::out_has_data_loaded
     *   is set.
    */
    void on_tx(tx_info_t& info)
    {
        auto slot_number = info.in_frame_info.slot_number;
        if ((is_rx_enabled || is_tx_enabled) && last_stats_slot != slot_number &&
            (info.in_frame_info.slot_number & 0x7FF) == 0x7FF)
        {
            last_stats_slot = slot_number;
            print_stats();
        }

        std::unique_lock<std::mutex> lg(tx_mutex);
        if (!is_tx_enabled)
        {
            return;
        }

        check_retransmit(slot_number);

        // @note Setup tx_ring element for retransmit check
        auto ack_ck_idx = tx_ring_index(slot_number + tx_config.arq_window_size);
        auto tx_idx = tx_ring_index(slot_number);
        auto& ack_elem = tx_ring[ack_ck_idx];
        auto& tx_elem = tx_ring[tx_idx];

        llc_t llc(info.out_pdu.base, info.out_pdu.size);
        llc.crc_size = tx_crc_size;

        // @note tx_info for slot synchronization only
        if (info.out_pdu.size <= llc.get_header_size())
        {
            info.out_pdu.size = 0;
            // @note No allocation just inform PDCP for slot update;
            pdcp->on_tx(info);
            return;
        }

        info.out_pdu.base = llc.payload();
        info.out_pdu.size -= llc.get_header_size();

        {
            // @note Allocate LLC-DATA-ACKs first
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

                    Logless(*main_logger, LLC_TRC, "TRC | LLC#   | ack sn=# cx=#",
                        int(lcid), ack.first, ack.second);

                    n_acks++;
                }
                
                llc.set_payload_size(n_acks*sizeof(llc_payload_ack_t));
                llc.set_SN(0);
                llc.set_A(true);
                llc.set_LCID(lcid);
                if (tx_crc_size)
                {
                    // @todo : Calculate CRC for this LLC
                }
                info.out_allocated += llc.get_SIZE();
                llc.base = llc.base + llc.get_SIZE();
            }
        }

        // @note No allocation just inform PDCP for slot update;
        if (info.out_pdu.size < llc.get_header_size() || !info.in_allow_data)
        {
            info.out_pdu.size = 0;
            pdcp->on_tx(info);
            return;
        }

        llc.set_LCID(lcid);
        llc.set_R(false);
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
                pdcp->on_tx(info);
                info.out_tx_available = true;
                return;
            }
        }

        size_t init_alloc = info.out_allocated;
        size_t retx_count = 0;
        // @note Get PDCP from PDCP layer
        if (!has_retransmit)
        {
            pdcp->on_tx(info);
            size_t pdcp_allocation_size = info.out_allocated - init_alloc;
            llc.set_payload_size(pdcp_allocation_size);
            info.out_has_data_loaded = true;
        }
        // @note Get PDCP from retransmit
        else
        {
            auto& retx_pdu = to_retx_list.front();
            retx_count = retx_pdu.retry_count + 1;

            Logless(*main_logger, LLC_TRC,
                "TRC | LLC#   | retx=#/# to_retx_list_sz=#",
                int(lcid),
                retx_count,
                tx_config.max_retx_count,
                to_retx_list.size());

            if (retx_count >= tx_config.max_retx_count)
            {
                info.out_allocated = 0;
                lg.unlock();
                rrc.on_rlf(lcid);
                return;
            }
            else
            {
                std::memcpy(llc.payload(), retx_pdu.pdcp_pdu.data(), retx_pdu.pdcp_pdu_size);
                free_buffer(std::move(retx_pdu.pdcp_pdu));
                llc.set_payload_size(retx_pdu.pdcp_pdu_size);
                info.out_allocated += retx_pdu.pdcp_pdu_size;
                to_retx_list.pop_front();

                info.out_pdu.size = 0;
                pdcp->on_tx(info);
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

        if (E_TX_MODE_AM == tx_config.mode)
        {
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
            stats.pkt_resent.fetch_add(1);
            stats.bytes_resent.fetch_add(tx_elem.pdcp_pdu_size);
        }

        Logless(*main_logger, LLC_TRC,
            "TRC | LLC#   | pdu slot=# tx_idx=# ack_idx=# sn=# pdu_sz=#",
            (int) lcid,
            slot_number,
            tx_idx,
            ack_ck_idx,
            (int) llc.get_SN(),
            tx_elem.pdcp_pdu_size);
    }

    void on_rx(rx_info_t& info)
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        if (!is_rx_enabled)
        {
            return;
        }

        llc_t llc(info.in_pdu.base, info.in_pdu.size);
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
                    Logless(*main_logger, LLC_ERR, "ERR | LLC#   | ack overrun, llc_size=# ack_idx=#",
                        int(lcid), llc_size, i);
                    break;
                }

                auto& ack = acks[i];
                llc_sn_t sn = ack.sn & llc_sn_mask;
                for (size_t i=0; i<ack.count; i++)
                {
                    size_t idx_ = sn_to_tx_ring[sn];
                    auto idx = tx_ring_index(idx_);
                    auto& ack_slot = tx_ring[idx];
                    auto sent_idx = tx_ring_index(ack_slot.sent_index);
                    auto& sent_slot = tx_ring[sent_idx];
                    sent_slot.acknowledged = true;

                    stats.pkt_sent.fetch_add(1);
                    stats.bytes_sent.fetch_add(sent_slot.pdcp_pdu_size);

                    Logless(*main_logger, LLC_TRC,
                        "TRC | LLC#   | acked idx=# sn=#",
                        (int) lcid, ack_slot.sent_index,
                        (int) sn);
                    sn = llc_sn_mask & (sn+1);
                }
            }
        }
        // @note Handle data
        else
        {
            if (rx_config.peer_mode == ILLC::E_TX_MODE_AM)
            {
                to_acknowledge(llc.get_SN());
            }

            info.in_pdu.base += llc.get_header_size();
            info.in_pdu.size -= llc.get_header_size();

            Logless(*main_logger, LLC_TRC, "TRC | LLC#   | data rx pdu_sz=#",
                int(lcid), info.in_pdu.size);
 
            stats.bytes_recv.fetch_add(info.in_pdu.size);
            stats.pkt_recv.fetch_add(1);
            pdcp->on_rx(info);
        }
    }

private:
    void check_retransmit(size_t slot_number)
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
            "TRC | LLC#   | retxing current_slot=# sent_index=#",
            int(lcid),
            slot_number,
            this_slot.sent_index);
 
        // @note Reset state to default
        sent_slot.acknowledged = true;
        // @note Queue back retx PDCP
        to_retx_list.emplace_back();
        auto& to_retx = to_retx_list.back();
        to_retx.retry_count = sent_slot.retry_count;
        to_retx.pdcp_pdu_size = sent_slot.pdcp_pdu_size;
        to_retx.pdcp_pdu = allocate_buffer();
        sent_slot.pdcp_pdu.swap(to_retx.pdcp_pdu);
        stats.pkt_resent.fetch_add(1);
        stats.bytes_resent.fetch_add(sent_slot.pdcp_pdu_size);
    }

    void to_acknowledge(llc_sn_t sn)
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

    void increment_sn()
    {
        sn_counter = (sn_counter+1) & 0x3F;
    }

    size_t tx_ring_index(size_t slot)
    {
        return slot % tx_ring.size();
    }

    buffer_t allocate_buffer()
    {
        if (buffer_pool.size())
        {
            auto rv = std::move(buffer_pool.back());
            buffer_pool.pop_back();
            return std::move(rv);
        }

        return buffer_t(llc_max_size);
    }

    void free_buffer(buffer_t&& buffer)
    {
        buffer_pool.emplace_back(std::move(buffer));
    }

    const stats_t& get_stats()
    {
        return stats;
    }

    // @brief Used to indicated retransmit slot
    struct tx_ring_elem_t
    {
        size_t sent_index;
        size_t retry_count;
        size_t pdcp_pdu_size;
        buffer_t pdcp_pdu;
        bool acknowledged;
    };

    struct retx_elem_t
    {
        size_t retry_count;
        size_t pdcp_pdu_size;
        buffer_t pdcp_pdu;
    };

    std::shared_ptr<IPDCP> pdcp;
    IRRC& rrc;

    // @brief Used for pdpc cache and retx cache
    std::list<buffer_t> buffer_pool;
    // @brief Logical Channel Identifier
    lcid_t lcid;
    tx_config_t tx_config;
    rx_config_t rx_config;

    size_t tx_crc_size = 0;
    size_t rx_crc_size = 0;

    // @brief Context for the checking of acknowledgments and PDCP cache
    std::vector<tx_ring_elem_t> tx_ring;
    // @brief Mapping for sn to tx_ring index
    std::vector<size_t> sn_to_tx_ring;
    // @brief LLC sequence number list pending for acknowledgement
    std::list<std::pair<size_t,size_t>> to_ack_list;
    // @brief PDCP frames that needs retransmission (multithreaded)
    std::list<retx_elem_t> to_retx_list;
    std::mutex to_ack_list_mutex;
    llc_sn_t  sn_counter = 0;
    static constexpr size_t llc_max_size = 1500;

    bool is_tx_enabled = false;
    bool is_rx_enabled = false;
    std::mutex tx_mutex;
    std::mutex rx_mutex;

    stats_t stats;
    uint64_t last_stats_slot = -1;
};

#endif // __WINJECTUM_LLC_HPP__
