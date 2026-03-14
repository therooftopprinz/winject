/*
 * PDCP DL helper implementation
 */

#include "pdcp_dl.hpp"

#include <cstring>
#include <iostream>

namespace winject
{

pdcp_dl::pdcp_dl(const pdcp_dl_config_t& cfg)
    : config(cfg)
{
}

pdcp_dl::~pdcp_dl() = default;

void pdcp_dl::reset()
{
    reassembly_map.clear();
    completed_frames.clear();
    outstanding_bytes = 0;
    status            = STATUS_CODE_SUCCESS;
    next_expected_sn  = 0;
}

void pdcp_dl::reconfigure(const pdcp_dl_config_t& cfg)
{
    config = cfg;
    // Changing reassembly behaviour invalidates current partial state.
    reassembly_map.clear();
    next_expected_sn = 0;
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

bool pdcp_dl::on_pdcp_data(bfc::const_buffer_view pdcp)
{
    if (pdcp.empty())
    {
        std::cout << "pdcp_dl: empty buffer\n";
        status = STATUS_CODE_BUFFER_INVALID_DATA;
        return false;
    }

    size_t available_data = pdcp.size();
    auto*  cursor = reinterpret_cast<const uint8_t*>(pdcp.data());

    bool processed_any_segment = false;

    while (available_data)
    {
        pdcp_segment_const_t segment(cursor, available_data);
        segment.has_sn     = config.allow_reordering;
        segment.has_offset = config.allow_segmentation;
        segment.rescan();

        if (!segment.is_header_valid())
        {
            std::cout << "pdcp_dl: invalid header\n";
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const size_t header_size = segment.get_header_size();
        const size_t total_size = static_cast<size_t>(segment.get_SIZE());

        if (header_size >= available_data ||
            total_size > available_data ||
            total_size < header_size)
        {
            std::cout << "pdcp_dl: size mismatch "
                      << "header_size=" << header_size
                      << " total_size=" << total_size
                      << " available_data=" << available_data << "\n";
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const auto payload_size = static_cast<size_t>(segment.get_payload_size());

        if (!payload_size)
        {
            std::cout << "pdcp_dl: zero payload size\n";
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        const pdcp_sn_t sn = segment.get_SN().value_or(static_cast<pdcp_sn_t>(0u));
        const auto offset  = static_cast<size_t>(segment.get_OFFSET().value_or(0u));

        if (!config.allow_segmentation && offset != 0u)
        {
            std::cout << "pdcp_dl: segmentation disabled but offset!=0\n";
            status = STATUS_CODE_BUFFER_INVALID_DATA;
            return false;
        }

        auto& state = reassembly_map[sn];

        const size_t required_size = offset + payload_size;
        if (state.buffer.capacity() < required_size)
        {
            state.buffer.resize(required_size);
        }

        const bool is_duplicate_offset = !state.recvd_offsets.emplace(offset).second;

        if (!is_duplicate_offset)
        {
            std::memcpy(
                state.buffer.data() + offset,
                segment.payload,
                payload_size);
            state.accumulated_size += payload_size;
        }

        if (segment.is_LAST())
        {
            state.expected_size     = required_size;
            state.has_expected_size = true;
        }

        if (state.has_expected_size &&
            state.accumulated_size == state.expected_size)
        {
            state.is_complete = true;

            // When reordering is disabled, we can emit a frame as soon
            // as its segments are complete without waiting for others.
            if (!config.allow_reordering)
            {
                outstanding_bytes += state.expected_size;
                completed_frames.emplace_back(std::move(state.buffer));
                state = reassembly_state_t{};
            }
        }

        cursor         += total_size;
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
            std::cout << "pdcp_dl: no segment processed\n";
            status = STATUS_CODE_BUFFER_INVALID_DATA;
        }
        return false;
    }

    if (status == STATUS_CODE_SUCCESS)
    {
        status = STATUS_CODE_SUCCESS;
    }

    return true;
}

void pdcp_dl::flush_completed_in_order()
{
    while (true)
    {
        auto it = reassembly_map.find(next_expected_sn);
        if (it == reassembly_map.end() || !it->second.is_complete)
        {
            break;
        }

        auto& state = it->second;
        outstanding_bytes += state.expected_size;
        completed_frames.emplace_back(std::move(state.buffer));
        reassembly_map.erase(it);

        next_expected_sn = static_cast<pdcp_sn_t>(next_expected_sn + 1);
    }
}

} // namespace winject

