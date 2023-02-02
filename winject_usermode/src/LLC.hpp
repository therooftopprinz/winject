#ifndef __WINJECTUM_LLC_HPP__
#define __WINJECTUM_LLC_HPP__

#include "ILLC.hpp"
#include "IPDCP.hpp"
#include "frame_defs.hpp"
#include <memory>
#include <optional>
#include <cstring>
#include <list>
#include <mutex>

class LLC : public ILLC
{
public:
    LLC(
        std::shared_ptr<IPDCP> pdcp,
        uint8_t lcid,
        size_t retx_slot_count,
        size_t retx_max_count)
        : pdcp(pdcp)
        , lcid(lcid)
        , retx_slot_count(retx_slot_count)
        , retx_max_count(retx_max_count)
        , tx_ring(64)
    {
        reset_tx();
    }

    void reset_tx()
    {

        for (auto& tx_ring_elem : tx_ring)
        {
            if(tx_ring_elem.pdcp_pdu.empty())
            {
                tx_ring_elem.pdcp_pdu = allocate_buffer();
            }
            tx_ring_elem.acknowledged = true;
            tx_ring_elem.to_check = -1;
        }

        for (auto& i : to_retx_list)
        {
            if (i.pdcp_pdu.size())
            {
                free_buffer(std::move(i.pdcp_pdu));
            }
        }

        to_retx_list.clear();
    }

    /**
     * @brief on_tx
     *   Called by scheduler to fillup buffer with LLC PDUs
     *   Should not be called again once tx_info_t::has_data_loaded
     *   is set.
    */
    void on_tx(tx_info_t& info)
    {
        check_retransmit(info.in_frame_info.slot_number);

        // @note Setup tx_ring element for retransmit check
        auto slot_number = info.in_frame_info.slot_number;
        auto ack_ck_idx = tx_ring_index(slot_number + retx_slot_count);
        auto tx_idx = tx_ring_index(slot_number);
        auto& ack_elem = tx_ring[ack_ck_idx];
        auto& tx_elem = tx_ring[tx_idx];

        // @note tx_info for slot synchronization only
        if (info.out_pdu.size <= llc_t::get_header_size())
        {
            info.out_pdu.size = 0;
            // @note No allocation just inform PDCP for slot update;
            pdcp->on_tx(info);
            return;
        }

        llc_t llc(info.out_pdu.base, info.out_pdu.size);

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

                    n_acks++;
                }
                
                llc.set_payload_size(n_acks*sizeof(llc_payload_ack_t));
                llc.set_SN(0);
                llc.set_D(true);
                llc.set_A(true);
                llc.set_LCID(lcid);
                info.out_allocated += llc.get_SIZE();
                llc.base = llc.base + llc.get_SIZE();
            }
        }

        // @note Allocate PDCP
        if (info.out_pdu.size < llc_t::get_header_size())
        {
            info.out_pdu.size = 0;
            // @note No allocation just inform PDCP for slot update;
            pdcp->on_tx(info);
            return;
        }

        llc.set_LCID(lcid);
        llc.set_D(true);
        llc.set_R(false);
        llc.set_A(false);
        info.out_pdu.base = llc.payload();
        info.out_pdu.size -= llc_t::get_header_size();

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
                info.tx_available = true;
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
            info.has_data_loaded = true;
        }
        // @note Get PDCP from retransmit
        else
        {
            auto& retx_pdu = to_retx_list.front();
            retx_count = retx_pdu.retry_count + 1;
            if (retx_count >= retx_max_count)
            {
                info.out_pdu.size = 0;
                // @note No allocation just inform PDCP for slot update;S
                pdcp->on_tx(info);
                pdcp->on_tx_rlf();
                info.out_allocated = 0;
                on_tx_rlf();
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
                info.has_data_loaded = true;
            }
        }

        // @note No PDCP allocated
        if (init_alloc == info.out_allocated)
        {
            return;
        }

        info.out_allocated += llc_t::get_header_size();

        llc.set_SN(sn_counter);
        increment_sn();

        tx_elem.retry_count = retx_count;
        tx_elem.acknowledged = false;
        ack_elem.to_check = tx_idx;

        // @note copy PDCP to tx_ring elem needed when retransmitting
        std::memcpy(tx_elem.pdcp_pdu.data(), llc.payload(), llc.get_payload_size());
        tx_elem.pdcp_pdu_size = llc.get_payload_size();
    }

    void on_rx(rx_info_t& info)
    {
        llc_t llc(info.in_pdu.base, info.in_pdu.size);
        // @note Handle ack
        if (llc.get_D() && llc.get_A())
        {
            auto idx = tx_ring_index(info.in_frame_info.slot_number);
            auto& current_slot = tx_ring[idx];
            auto sent_idx = tx_ring_index(current_slot.to_check);
            auto& sent_slot = tx_ring[sent_idx];
            sent_slot.acknowledged = true;
        }
        // @note Handle data
        else if (llc.get_D())
        {
            to_acknowledge(llc.get_SN());
            info.in_pdu.base += llc_t::get_header_size();
            info.in_pdu.size -= llc_t::get_header_size();
            pdcp->on_rx(info);
        }
    }

    void on_rx_rlf()
    {
        {
            std::unique_lock<std::mutex> lg(to_ack_list_mutex);
            to_ack_list.clear();
        }

        pdcp->on_rx_rlf();
    }

private:
    void on_tx_rlf()
    {
        reset_tx();
    }

    void check_retransmit(size_t slot_number)
    {
        auto& this_slot = tx_ring[tx_ring_index(slot_number)];
        auto& sent_slot = tx_ring[tx_ring_index(this_slot.to_check)];
        if (sent_slot.acknowledged)
        {
            return;
        }
        // @note Reset state to default
        sent_slot.acknowledged = true;
        // @note Queue back retx PDCP
        to_retx_list.emplace_back();
        auto& to_retx = to_retx_list.back();
        to_retx.retry_count = sent_slot.retry_count;
        to_retx.pdcp_pdu_size = sent_slot.pdcp_pdu_size;
        to_retx.pdcp_pdu = allocate_buffer();
        sent_slot.pdcp_pdu.swap(to_retx.pdcp_pdu);
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

    struct tx_ring_elem_t
    {
        size_t to_check;
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
    // @brief Used for pdpc cache and retx cache
    std::list<buffer_t> buffer_pool;
    // @brief Logical Channel Identifier
    lcid_t lcid;
    // @brief Number of slot before retransmit
    size_t retx_slot_count;
    // @brief Max retransmit before reporting radio link failure to PDCP
    size_t retx_max_count;
    // @brief Context for the checking of acknowledgments and PDCP cache
    std::vector<tx_ring_elem_t> tx_ring;
    // @brief LLC sequence number list pending for acknowledgement
    std::list<std::pair<size_t,size_t>> to_ack_list;
    // @brief PDCP frames that needs retransmission (multithreaded)
    std::list<retx_elem_t> to_retx_list;
    std::mutex to_ack_list_mutex;
    llc_sn_t  sn_counter = 0;
    const size_t llc_max_size = 1500;
};

#endif // __WINJECTUM_LLC_HPP__
