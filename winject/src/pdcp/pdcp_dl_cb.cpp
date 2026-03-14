/*
 * PDCP DL circular-buffer implementation
 */

#include "pdcp_dl_cb.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace winject
{

pdcp_dl_cb::pdcp_dl_cb(const pdcp_dl_cb_config_t& cfg)
    : config(cfg)
{
    const size_t len = config.reorder_buffer_len > 0 ? config.reorder_buffer_len : 1;
    reorder_ring.resize(len);
}

pdcp_dl_cb::~pdcp_dl_cb() = default;

void pdcp_dl_cb::reset()
{
    for (auto& slot : reorder_ring)
    {
        slot.valid = false;
    }
    overflow_deque.clear();
    completed_frames.clear();
    outstanding_bytes = 0;
    status             = STATUS_CODE_SUCCESS;
    next_expected_sn   = 0;
}

void pdcp_dl_cb::reconfigure(const pdcp_dl_cb_config_t& cfg)
{
    config = cfg;
    const size_t len = config.reorder_buffer_len > 0 ? config.reorder_buffer_len : 1;
    reorder_ring.resize(len);
    for (auto& slot : reorder_ring)
    {
        slot.valid = false;
    }
    overflow_deque.clear();
    next_expected_sn = 0;
}

pdcp_dl_cb_config_t pdcp_dl_cb::get_config()
{
    return config;
}

pdcp_dl_cb::status_code_e pdcp_dl_cb::get_status()
{
    return status;
}

size_t pdcp_dl_cb::get_outstanding_bytes()
{
    return outstanding_bytes;
}

size_t pdcp_dl_cb::get_outstanding_packet()
{
    return completed_frames.size();
}

bfc::sized_buffer pdcp_dl_cb::pop()
{
    if (completed_frames.empty())
    {
        return {};
    }

    bfc::sized_buffer rv = std::move(completed_frames.front());
    completed_frames.pop_front();
    if (outstanding_bytes >= rv.size())
    {
        outstanding_bytes -= rv.size();
    }
    else
    {
        outstanding_bytes = 0;
    }

    return rv;
}

size_t pdcp_dl_cb::slot_index(pdcp_sn_t sn) const
{
    const size_t len = reorder_ring.size();
    return len ? (static_cast<size_t>(sn) % len) : 0;
}

pdcp_dl_cb::reassembly_state_t* pdcp_dl_cb::get_or_put_slot(pdcp_sn_t sn)
{
    if (reorder_ring.empty())
    {
        return nullptr;
    }
    const size_t idx = slot_index(sn);
    slot_t& slot     = reorder_ring[idx];
    if (slot.valid && slot.sn != sn)
    {
        return nullptr;
    }
    if (!slot.valid)
    {
        slot.sn    = sn;
        slot.valid = true;
        slot.state = reassembly_state_t{};
    }
    return &slot.state;
}

pdcp_dl_cb::reassembly_state_t* pdcp_dl_cb::find_overflow(pdcp_sn_t sn)
{
    for (auto& p : overflow_deque)
    {
        if (p.first == sn)
        {
            return &p.second;
        }
    }
    return nullptr;
}

void pdcp_dl_cb::flush_completed_in_order()
{
    while (true)
    {
        const size_t idx = slot_index(next_expected_sn);
        slot_t* slot    = (idx < reorder_ring.size()) ? &reorder_ring[idx] : nullptr;
        if (slot && slot->valid && slot->sn == next_expected_sn && slot->state.is_complete)
        {
            outstanding_bytes += slot->state.expected_size;
            completed_frames.emplace_back(std::move(slot->state.buffer));
            slot->valid = false;
            next_expected_sn = static_cast<pdcp_sn_t>(next_expected_sn + 1);
            continue;
        }

        auto it = std::find_if(overflow_deque.begin(), overflow_deque.end(),
            [this](const std::pair<pdcp_sn_t, reassembly_state_t>& p) {
                return p.first == next_expected_sn && p.second.is_complete;
            });
        if (it != overflow_deque.end())
        {
            outstanding_bytes += it->second.expected_size;
            completed_frames.emplace_back(std::move(it->second.buffer));
            overflow_deque.erase(it);
            next_expected_sn = static_cast<pdcp_sn_t>(next_expected_sn + 1);
            continue;
        }

        break;
    }
}

bool pdcp_dl_cb::on_pdcp_data(bfc::const_buffer_view pdcp)
{
    if (pdcp.empty())
    {
        status = STATUS_CODE_BUFFER_INVALID_DATA;
        return false;
    }

    size_t available_data = pdcp.size();
    auto*  cursor        = reinterpret_cast<const uint8_t*>(pdcp.data());

    bool processed_any_segment = false;

    while (available_data)
    {
        pdcp_segment_const_t segment(cursor, available_data);
        segment.has_sn     = config.allow_reordering;
        segment.has_offset = config.allow_segmentation;
        segment.rescan();

        if (!segment.is_header_valid())
        {
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const size_t header_size = segment.get_header_size();
        const size_t total_size  = static_cast<size_t>(segment.get_SIZE());

        if (header_size >= available_data ||
            total_size > available_data ||
            total_size < header_size)
        {
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const auto payload_size = static_cast<size_t>(segment.get_payload_size());

        if (!payload_size)
        {
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const pdcp_sn_t sn    = segment.get_SN().value_or(static_cast<pdcp_sn_t>(0u));
        const auto offset    = static_cast<size_t>(segment.get_OFFSET().value_or(0u));

        if (!config.allow_segmentation && offset != 0u)
        {
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        bool state_from_overflow = false;
        reassembly_state_t* state = get_or_put_slot(sn);
        if (!state)
        {
            state = find_overflow(sn);
            if (!state)
            {
                overflow_deque.emplace_back(sn, reassembly_state_t{});
                state = &overflow_deque.back().second;
            }
            state_from_overflow = true;
        }

        const size_t required_size = offset + payload_size;
        if (state->buffer.capacity() < required_size)
        {
            state->buffer.resize(required_size);
        }

        const bool is_duplicate_offset = !state->recvd_offsets.emplace(offset).second;

        if (!is_duplicate_offset)
        {
            std::memcpy(
                state->buffer.data() + offset,
                segment.payload,
                payload_size);
            state->accumulated_size += payload_size;
        }

        if (segment.is_LAST())
        {
            state->expected_size     = required_size;
            state->has_expected_size = true;
        }

        if (state->has_expected_size &&
            state->accumulated_size == state->expected_size)
        {
            state->is_complete = true;

            if (!config.allow_reordering)
            {
                outstanding_bytes += state->expected_size;
                completed_frames.emplace_back(std::move(state->buffer));
                *state = reassembly_state_t{};
                if (state_from_overflow)
                {
                    for (auto it = overflow_deque.begin(); it != overflow_deque.end(); ++it)
                    {
                        if (it->first == sn)
                        {
                            overflow_deque.erase(it);
                            break;
                        }
                    }
                }
                else if (reorder_ring.size())
                {
                    const size_t idx = slot_index(sn);
                    if (idx < reorder_ring.size() && reorder_ring[idx].sn == sn)
                    {
                        reorder_ring[idx].valid = false;
                    }
                }
            }
        }

        cursor += total_size;
        available_data -= total_size;
        processed_any_segment = true;
    }

    if (config.allow_reordering)
    {
        flush_completed_in_order();
    }

    if (!processed_any_segment)
    {
        if (status == STATUS_CODE_SUCCESS)
        {
            status = STATUS_CODE_BUFFER_INVALID_DATA;
        }
        return false;
    }

    return true;
}

} // namespace winject
