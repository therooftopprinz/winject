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

#include "pdcp_dl.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>

namespace winject
{

pdcp_dl::pdcp_dl(const pdcp_dl_config_t& cfg)
    : config(cfg)
    , reorder_ring(cfg.reorder_size)
{}

pdcp_dl::~pdcp_dl() = default;

void pdcp_dl::reset()
{
    reorder_ring.clear();
    reorder_ring.resize(config.reorder_size);

    overflow_deque.clear();
    completed_frames.clear();
    outstanding_bytes  = 0;
    status             = STATUS_CODE_SUCCESS;
    next_expected_sn   = 0;
}

void pdcp_dl::reconfigure(const pdcp_dl_config_t& cfg)
{
    config = cfg;
    reset();
}

pdcp_dl_config_t pdcp_dl::get_config()
{
    return config;
}

pdcp_dl::status_code_e pdcp_dl::get_status()
{
    return status;
}

size_t pdcp_dl::get_outstanding_bytes()
{
    return outstanding_bytes;
}

size_t pdcp_dl::get_outstanding_packet()
{
    return completed_frames.size();
}

bfc::sized_buffer pdcp_dl::pop()
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

size_t pdcp_dl::slot_index(pdcp_sn_t sn) const
{
    const size_t len = reorder_ring.size();
    return len ? (static_cast<size_t>(sn) % len) : 0;
}

pdcp_dl::reassembly_state_t* pdcp_dl::get_or_put_slot(pdcp_sn_t sn)
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

pdcp_dl::reassembly_state_t* pdcp_dl::find_overflow(pdcp_sn_t sn)
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

void pdcp_dl::flush_completed_in_order()
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

bool pdcp_dl::on_pdcp_data(bfc::const_buffer_view pdcp)
{
    if (pdcp.empty())
    {
        status = STATUS_CODE_INPUT_EMPTY;
        return false;
    }

    size_t available_data = pdcp.size();
    auto*  cursor        = reinterpret_cast<const uint8_t*>(pdcp.data());

    while (available_data)
    {
        pdcp_segment_const_t segment(cursor, available_data, config.allow_reordering, config.allow_segmentation);
        segment.rescan();

        if (!segment.is_header_valid())
        {
            status = STATUS_CODE_SEGMENT_SCAN_FAILED;
            return false;
        }

        const size_t header_size = segment.get_header_size();
        const size_t total_size  = static_cast<size_t>(segment.get_SIZE());

        if (total_size > available_data)
        {
            status = STATUS_CODE_SEGMENT_OUTSIDE_INPUT;
            return false;
        }

        const auto payload_size = static_cast<size_t>(segment.get_payload_size());

        if (!payload_size)
        {
            status = STATUS_CODE_RESERVED_VALUE_USED;
            return false;
        }

        const pdcp_sn_t             sn     = config.allow_reordering   ? segment.get_SN() : 0;
        const pdcp_segment_offset_t offset = config.allow_segmentation ? segment.get_OFFSET() : 0;

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
                segment.get_payload(),
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
    }

    if (config.allow_reordering)
    {
        flush_completed_in_order();
    }

    return true;
}

} // namespace winject
