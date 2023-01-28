#ifndef __LLC_HPP__
#define __LLC_HPP__

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
    LLC(std::shared_ptr<IPDCP> pdcp)
        : pdcp(pdcp)
        , retx_slot_count(5)
        , retx_max_count(4)
    {}

    void on_tx(tx_info_t& info)
    {
        check_retransmit(info.in_frame_info->slot_number);

        // @note tx_info for slot synchronization only
        if (info.out_pdu.size <= llc_t::get_header_size())
        {
            return;
        }

        llc_t llc(info.out_pdu.base, info.out_pdu.size);

        info.out_pdu.base = llc.payload();
        info.out_pdu.size -= llc.get_header_size();

        // @note Allocate ACKs first
        {
            std::unique_lock<std::mutex> lg(to_ack_list_mutex);
            llc_payload_ack_t* acks = (llc_payload_ack_t*) llc.payload();

            if (to_ack_list.size())
            {
                int n_acks = 0;
                while (info.out_pdu.size >= sizeof(llc_payload_ack_t))
                {
                    lg.lock();
                    if (to_ack_list.size())
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
            return;
        }

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
            if (retx_pdu.first > info.out_pdu.size)
            {
                return;
            }
        }

        size_t init_alloc = info.out_allocated; 
        // @note Get PDCP from PDCP layer
        if (!has_retransmit)
        {
            pdcp->on_tx(info);
        }
        // @note Get PDCP from retransmit
        else
        {
            auto& retx_pdu = to_retx_list.front();
            std::memcpy(llc.payload(), retx_pdu.second.data(), retx_pdu.first);
            free_buffer(std::move(retx_pdu.second));
            llc.set_payload_size(retx_pdu.first);
            info.out_allocated += retx_pdu.first;
            to_retx_list.pop_front();
        }

        // @note No PDCP allocated
        if (info.out_allocated == info.out_allocated)
        {
            return;
        }

        info.out_allocated += llc_t::get_header_size();

        llc.set_SN(sn_counter);
        increment_sn();

        // @note Setup tx_ring element for retransmit check
        auto slot_number = info.in_frame_info->slot_number;
        auto ack_ck_idx = tx_ring_index(slot_number + retx_slot_count);
        auto tx_idx = tx_ring_index(slot_number);
        auto& ack_elem = tx_ring[ack_ck_idx];
        auto& tx_elem = tx_ring[tx_idx];

        tx_elem.acknowledged = false;
        tx_elem.ref_check = ack_ck_idx;
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
            auto idx = tx_ring_index(info.in_frame_info->slot_number);
            auto& sent = tx_ring[idx];
            sent.acknowledged = true;

            // @note Reset sent tx_ring_elem
            auto& to_check_slot = tx_ring[*(sent.to_check)];
            // @note Reset to_check tx_ring_elem
            to_check_slot.ref_check.reset();
            sent.to_check.reset();
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

private:
    void check_retransmit(size_t slot_number)
    {

    }

    void to_acknowledge(size_t sn)
    {
        std::unique_lock<std::mutex> lg(to_ack_list_mutex);
        if (!to_ack_list.size())
        {
            to_ack_list.emplace_front(sn, 1);
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
                to_ack_list.emplace_front(sn, 1);
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

    struct tx_ring_elem_t
    {
        std::optional<int> to_check;
        bool acknowledged;
        std::optional<int> ref_check;
        buffer_t pdcp_pdu;
        size_t pdcp_pdu_size;
    };

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

    std::shared_ptr<IPDCP> pdcp;
    // @brief Used for pdpc cache and retx cache
    std::list<buffer_t> buffer_pool;
    // @brief Logical Channel Identifier
    uint8_t lcid;
    // @brief Number of slot before retransmit
    size_t retx_slot_count;
    // @brief Max retransmit before reporting radio link failure to PDCP
    size_t retx_max_count;
    // @brief Context for the checking of acknowledgments and PDCP cache
    std::vector<tx_ring_elem_t> tx_ring;
    // @brief LLC sequence number list pending for acknowledgement
    std::list<std::pair<size_t,size_t>> to_ack_list;
    // @brief PDCP frames that needs retransmission (multithreaded)
    std::list<std::pair<size_t,buffer_t>> to_retx_list;
    std::mutex to_ack_list_mutex;
    uint8_t  sn_counter;
    const size_t llc_max_size = 1500;
};

#endif // __LLC_HPP__
