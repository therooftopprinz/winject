#ifndef __LLC_HPP__
#define __LLC_HPP__

#include "ILLC.hpp"
#include "IPDCP.hpp"
#include "frame_defs.hpp"
#include <memory>
#include <optional>
#include <cstring>

class LLC : public ILLC
{
public:
    LLC(std::shared_ptr<IPDCP> pdcp)
        : pdcp(pdcp)
        , pending_size_max(10)
        , retx_slot_size(5)
        , retx_max_count(4)
    {}

    void get_pdu(pdu_t& pdu, size_t slot)
    {
        if (pending_size >= pending_size_max)
        {
            return;
        }
        
        uint8_t *llc_base = pdu.base;
        size_t llc_max_size = pdu.size;

        llc_t llc(llc_base, llc_max_size);
        llc.set_D(true);
        llc.set_R(false);
        llc.set_SN(slot);
        llc.set_C(false);

        pdu.base = llc.payload();
        pdu.size = llc.get_max_payload_size();

        pdcp->get_pdu(pdu, slot);
        if (pdu.size == llc_max_size)
        {
            return;
        }

        auto ack_ck_idx = ring_index(slot+retx_slot_size);
        auto tx_idx = ring_index(slot);
        auto& ack_elem = tx_ring[ack_ck_idx];
        auto& tx_elem = tx_ring[tx_idx];

        tx_elem.acknowledged = false;
        tx_elem.current_check = ack_ck_idx;
        ack_elem.to_check = tx_idx;
        std::memcpy(tx_elem.llc.data(), llc.payload(), pdu.size);
    }

    void get_prio_pdu(pdu_t& pdu, size_t slot)
    {
        auto current_idx = ring_index(slot);
        auto& current_elem = tx_ring[current_idx];
        if (!current_elem.to_check)
        {
            return;
        }
    }

    void on_pdu(const pdu_t& pdu, size_t slot)
    {}

private:
    size_t ring_index(size_t slot)
    {
        return slot % tx_ring.size();
    }

    struct tx_ring_elem_t
    {
        std::optional<int> to_check;
        bool acknowledged;
        std::optional<int> current_check;
        std::vector<uint8_t> llc;
    };

    std::shared_ptr<IPDCP> pdcp;

    uint8_t lcid = 0;
    size_t pending_size_max;
    size_t pending_size = 0;
    size_t retx_slot_size;
    size_t retx_max_count;
    size_t current_slot = 0;
    std::vector<tx_ring_elem_t> tx_ring;
};

#endif // __LLC_HPP__