#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <list>

#include <llc/llc_ul.hpp>
#include <crc.hpp>
#include <safeint.hpp>

namespace winject
{

llc_ul::llc_ul(const llc_ul_config_t& config)
    : config(config)
{
    reset();
    validate_config();
}

llc_ul::~llc_ul()
{}

void llc_ul::reconfigure(const llc_ul_config_t& c)
{
    config = c;
    reset();
    validate_config();
    
}

llc_ul_config_t llc_ul::get_config()
{
    return config;
}

void llc_ul::reset()
{
    status = 0;
    current_slot_number = 0;
    sn_counter = 0;
    tx_ring.clear();
    tx_ring.resize(config.tx_size);
    sn_ring.clear();
    sn_ring.resize(256);
    retx_list.clear();
    used_tx_slots = 0;
    pending_llc_pdu_count = 0;
}

bool llc_ul::update_current_slot_number(size_t slot)
{
    if (config.mode == E_LLC_TX_MODE_TM)
    {
        return true;
    }

    if (slot != current_slot_number+1)
    {
        set_sbit(STATUS_BIT_SLOT_SKIPPED);
        return false;
    }

    clear_sbit(STATUS_BIT_DATA_PDU_ALLOCATED);

    current_slot_number = slot;

    auto& old_slot = tx_ring[tx_ring_index(current_slot_number)];
    if (!old_slot.acknowledged)
    {
        old_slot.acknowledged  = true;
        used_tx_slots--;

        auto& sn_elem = sn_ring[sn_ring_index(old_slot.sn)];
        clear_sbit(STATUS_BIT_SHOULD_NOT_HAPPEN);
        clear_sbit(STATUS_BIT_SPURIOUS_ACK_SN);
        if (!sn_elem)
        {
            set_sbit(STATUS_BIT_SPURIOUS_ACK_SN | STATUS_BIT_SHOULD_NOT_HAPPEN);
            return false;
        }

        sn_elem->check_tx_slot_number.reset();

        retx_list.emplace_back();
        retx_elem_t& retx_elem = retx_list.back();
        retx_elem.sn           = old_slot.sn;
        retx_elem.retry_count  = old_slot.retry_count;
        retx_elem.llc_pdu      = std::move(old_slot.llc_pdu);
    }

    return true;
}

ssize_t llc_ul::get_free_tx_slot() const
{
    return tx_ring.size() - used_tx_slots;
}

ssize_t llc_ul::get_pending_llc_pdu_count() const
{
    return pending_llc_pdu_count;
}   

bool llc_ul::acknowledge(llc_sn_t sn)
{
    clear_sbit(STATUS_BIT_SPURIOUS_ACK_SN);
    clear_sbit(STATUS_BIT_SPURIOUS_ACK_TX);
    clear_sbit(STATUS_BIT_SHOULD_NOT_HAPPEN);

    auto& sn_elem = sn_ring[sn_ring_index(sn)];
    if (!sn_elem)
    {
        set_sbit(STATUS_BIT_SPURIOUS_ACK_SN);
        return false;
    }

    if (!sn_elem->check_tx_slot_number)
    {
        auto it = std::find_if(retx_list.begin(), retx_list.end(), [sn](const retx_elem_t& retx_elem) { return retx_elem.sn == sn; });
        if (it != retx_list.end())
        {
            retx_list.erase(it);
            sn_elem.reset();
            pending_llc_pdu_count--;
            return true;
        }

        set_sbit(STATUS_BIT_SPURIOUS_ACK_SN | STATUS_BIT_SHOULD_NOT_HAPPEN);
        return false;
    }

    tx_elem_t& tx_elem   = tx_ring[tx_ring_index(*sn_elem->check_tx_slot_number)];

    if (!tx_elem.acknowledged)
    {
        tx_elem.acknowledged = true;
        pending_llc_pdu_count--;
        used_tx_slots--;
        sn_elem.reset();
        return true;
    }

    // @note: can happen if ack got duplicated
    set_sbit(STATUS_BIT_SPURIOUS_ACK_TX);
    return false;
}

size_t llc_ul::recheck_slot_size(size_t retry_count) const
{
    return config.min_recheck_slot_number + retry_count * (config.max_recheck_slot_number - config.min_recheck_slot_number) / config.max_retx_count;
}

std::optional<size_t> llc_ul::enqueue_tx_check(bfc::shared_buffer_view&& llc_pdu, llc_sn_t sn, size_t retry_count)
{
    clear_sbit(STATUS_BIT_NO_TX_SLOT);
    size_t check_slot_number = current_slot_number + recheck_slot_size(retry_count);

    for (size_t i = 0; i < config.tx_size; i++)
    {
        size_t     check_slot_index = tx_ring_index(check_slot_number);
        tx_elem_t& check_slot       = tx_ring[check_slot_index];

        if (!check_slot.acknowledged)
        {
            check_slot_number++;
            continue;
        }

        // @note: actual move assign only happens here
        check_slot.llc_pdu      = std::move(llc_pdu);
        check_slot.sn           = sn;
        check_slot.retry_count  = retry_count;
        check_slot.acknowledged = false;

        used_tx_slots++;

        return check_slot_number;
    }

    set_sbit(STATUS_BIT_NO_TX_SLOT);
    return std::nullopt;
}

size_t llc_ul::get_retransmission_size() const
{
    if (retx_list.empty())
    {
        return 0;
    }

    return retx_list.front().llc_pdu.size();
}

size_t llc_ul::write_retransmission_pdu(bfc::buffer_view target)
{
    if (status & STATUS_BIT_DATA_PDU_ALLOCATED)
    {
        return 0;
    }

    if (config.mode == llc_tx_mode_e::E_LLC_TX_MODE_TM)
    {
        return 0;
    }

    size_t size = get_retransmission_size();
    if (size == 0)
    {
        return 0;
    }

    clear_sbit(STATUS_BIT_WRITE_BUFFER_TOO_SMALL);
    if (size > target.size())
    {
        set_sbit(STATUS_BIT_WRITE_BUFFER_TOO_SMALL);
        return 0;
    }

    auto& retx_elem = retx_list.front();
    auto& sn_elem = sn_ring[sn_ring_index(retx_elem.sn)];

    clear_sbit(STATUS_BIT_SPURIOUS_ACK_SN);
    clear_sbit(STATUS_BIT_SHOULD_NOT_HAPPEN);
    if (!sn_elem)
    {
        set_sbit(STATUS_BIT_SPURIOUS_ACK_SN | STATUS_BIT_SHOULD_NOT_HAPPEN);
        retx_list.pop_front();
        pending_llc_pdu_count--;
        return 0;
    }

    clear_sbit(STATUS_BIT_SN_LOST);
    if (retx_elem.retry_count >= config.max_retx_count)
    {
        set_sbit(STATUS_BIT_SN_LOST);
        sn_elem.reset();
        retx_list.pop_front();
        pending_llc_pdu_count--;
        return 0;
    }

    std::memcpy(target.data(), retx_elem.llc_pdu.data(), size);
    // @note: move will only hapen when enqueue_tx_check succeeds
    auto enqueue_result = enqueue_tx_check(std::move(retx_elem.llc_pdu), retx_elem.sn, retx_elem.retry_count + 1);
    if (!enqueue_result)
    {
        return 0;
    }

    sn_elem->check_tx_slot_number = *enqueue_result;

    retx_list.pop_front();

    set_sbit(STATUS_BIT_DATA_PDU_ALLOCATED);

    return size;
}

size_t llc_ul::get_llc_header_size() const
{
    llc_t llc(crc_size());
    return llc.get_header_size();
}

std::optional<llc_t> llc_ul::prepare_llc_pdu(bfc::shared_buffer_view buffer, bool is_ack)
{
    std::optional<llc_t> rv = llc_t(reinterpret_cast<uint8_t*>(buffer.data()), buffer.size(), crc_size());
    if (!rv->is_header_valid())
    {
        rv.reset();
        return rv;
    }

    rv->set_LCID(config.lcid);
    rv->set_SN(sn_counter);
    rv->set_A(is_ack);

    return rv;
}

size_t llc_ul::write_llc_pdu(bfc::shared_buffer_view target)
{
    if (status & STATUS_BIT_DATA_PDU_ALLOCATED)
    {
        return 0;
    }

    llc_t llc(reinterpret_cast<uint8_t*>(target.data()), target.size(), crc_size());

    std::optional<sn_elem_t>& sn_elem = sn_ring[sn_ring_index(sn_counter)];
    if (llc_tx_mode_e::E_LLC_TX_MODE_AM ==config.mode && !llc.get_A())
    {
        clear_sbit(STATUS_BIT_NO_FREE_SN);
        if (sn_elem)
        {
            set_sbit(STATUS_BIT_NO_FREE_SN);
            return 0;
        }
    }

    clear_sbit(STATUS_BIT_INVALID_LLC_PDU);
    if (!llc.is_valid())
    {
        set_sbit(STATUS_BIT_INVALID_LLC_PDU);
        return 0;
    }

    if (llc.get_A())
    {
        llc.set_SN(0);
    }
    else
    {
        llc.set_SN(sn_counter);
    }

    if(crc_size())
    {
        std::memset(llc.get_CRC(), 0, crc_size());
        winject::BEU32UA crc_actual = crc32_04C11DB7()(llc.get_base(), llc.get_SIZE());
        std::memcpy(llc.get_CRC(), &crc_actual,  std::min(crc_size(), sizeof(crc_actual)));
    }
    
    if (llc_tx_mode_e::E_LLC_TX_MODE_AM == config.mode && !llc.get_A())
    {
        auto tx_check_slot_number = enqueue_tx_check(std::move(target), sn_counter, 0);
        if (!tx_check_slot_number)
        {
            return 0;
        }
        
        sn_elem = sn_elem_t{*tx_check_slot_number};
        sn_counter++;
        pending_llc_pdu_count++;
        set_sbit(STATUS_BIT_DATA_PDU_ALLOCATED);
    }
    else
    {
        sn_counter++;
    }

    return llc.get_SIZE();
}

uint64_t llc_ul::get_status() const
{
    return status;
}

size_t llc_ul::crc_size() const
{
    if (config.crc_type == E_LLC_CRC_TYPE_CRC32_04C11DB7)
    {
        return 4;
    }
    return 0;
}

size_t llc_ul::tx_ring_index(size_t slot) const
{
    return slot % tx_ring.size();
}

size_t llc_ul::sn_ring_index(llc_sn_t sn) const
{
    return (sn & llc_sn_mask) % sn_ring.size();
}

void llc_ul::set_sbit(uint64_t bit)
{
    status |= bit;
}

void llc_ul::clear_sbit(uint64_t bit)
{
    status &= ~bit;
}

void llc_ul::validate_config()
{
    if (config.mode != llc_tx_mode_e::E_LLC_TX_MODE_TM &&
        config.mode != llc_tx_mode_e::E_LLC_TX_MODE_AM)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
    }

    if (config.crc_type != llc_crc_type_e::E_LLC_CRC_TYPE_NONE &&
        config.crc_type != llc_crc_type_e::E_LLC_CRC_TYPE_CRC32_04C11DB7)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }

    if (config.tx_size == 0 ||
        (config.tx_size & (config.tx_size - 1)) != 0)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }

    if (config.min_recheck_slot_number == 0)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }

    if (config.max_recheck_slot_number == 0)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }

    if (config.max_recheck_slot_number < config.min_recheck_slot_number)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }

    if (config.max_retx_count == 0)
    {
        set_sbit(STATUS_BIT_INVALID_CONFIG);
        return;
    }
}

} // namespace winject
