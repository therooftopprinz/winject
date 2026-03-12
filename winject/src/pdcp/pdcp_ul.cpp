/*
 * PDCP UL helper implementation
 */

#include "pdcp_ul.hpp"

#include <cstring>
#include <deque>

namespace winject
{

pdcp_ul::pdcp_ul(const pdcp_ul_config_t& cfg)
    : config(cfg)
{
}

pdcp_ul::~pdcp_ul() = default;

void pdcp_ul::reset()
{
    outstanding_buffers.clear();
    data_offset = 0;
    status = STATUS_CODE_SUCCESS;
    sequence_number = 0;
    total_in_bytes = 0;
    total_out_bytes = 0;
}

void pdcp_ul::reconfigure(const pdcp_ul_config_t& cfg)
{
    config = cfg;
}

pdcp_ul_config_t pdcp_ul::get_config()
{
    return config;
}

bool pdcp_ul::on_frame_data(bfc::buffer_view packet)
{
    if (packet.empty())
    {
        return false;
    }

    auto* data = new std::byte[packet.size()];
    std::memcpy(data, packet.data(), packet.size());
    outstanding_buffers.emplace_back(data, packet.size());
    total_in_bytes += packet.size();
    return true;
}

size_t pdcp_ul::get_outstanding_bytes()
{
    if (total_in_bytes < total_out_bytes)
    {
        return 0;
    }
    return total_in_bytes - total_out_bytes;
}

size_t pdcp_ul::get_outstanding_packet()
{
    return outstanding_buffers.size();
}

size_t pdcp_ul::get_min_commit_size()
{
    return config.min_commit_size;
}

pdcp_ul::status_code_e pdcp_ul::get_status()
{
    return status;
}

ssize_t pdcp_ul::write_pdcp(bfc::buffer_view buffer)
{
    if (buffer.empty())
    {
        status = STATUS_CODE_BUFFER_TOO_SMALL;
        return -1;
    }

    if (outstanding_buffers.empty())
    {
        status = STATUS_CODE_NO_DATA_TO_WRITE;
        return -1;
    }

    size_t available_for_data = buffer.size();
    uint8_t* payload_cursor = reinterpret_cast<uint8_t*>(buffer.data());

    size_t total_pdcp_written = 0;
    size_t segments_allocated = 0;

    while (available_for_data && !outstanding_buffers.empty())
    {
        const bool enable_reordering = config.allow_reordering;

        pdcp_segment_t segment(payload_cursor, available_for_data);
        segment.has_sn     = enable_reordering;
        segment.has_offset = enable_reordering;
        segment.rescan();

        if (!segment.is_header_valid())
        {
            break;
        }

        size_t max_segment_payload = available_for_data - segment.get_header_size();

        auto& front = outstanding_buffers.front();
        size_t frame_remaining = front.size() > data_offset ? (front.size() - data_offset) : 0;

        if (!frame_remaining)
        {
            outstanding_buffers.pop_front();
            data_offset = 0;
            continue;
        }

        size_t copy_size = frame_remaining;

        if (copy_size > max_segment_payload)
        {
            if (!config.allow_segmentation)
            {
                break;
            }
            copy_size = max_segment_payload;
        }

        std::memcpy(
            segment.payload,
            reinterpret_cast<uint8_t*>(front.data()) + data_offset,
            copy_size);

        if (enable_reordering)
        {
            const pdcp_segment_offset_t segment_offset =
                static_cast<pdcp_segment_offset_t>(data_offset);

            segment.set_SN(std::optional<pdcp_sn_t>(sequence_number));
            segment.set_OFFSET(std::optional<pdcp_segment_offset_t>(segment_offset));
        }

        data_offset     += copy_size;
        total_out_bytes += copy_size;

        bool is_last_segment_of_frame = (data_offset >= front.size());
        segment.set_LAST(is_last_segment_of_frame);
        segment.set_payload_size(static_cast<pdcp_segment_offset_t>(copy_size));

        size_t seg_size        = segment.get_SIZE();
        payload_cursor        += seg_size;
        available_for_data    -= seg_size;
        total_pdcp_written    += seg_size;
        segments_allocated++;

        if (is_last_segment_of_frame)
        {
            outstanding_buffers.pop_front();
            data_offset = 0;
            sequence_number++;
        }
    }

    if (!segments_allocated)
    {
        status = STATUS_CODE_BUFFER_TOO_SMALL;
        return -1;
    }

    status = STATUS_CODE_SUCCESS;
    return static_cast<ssize_t>(total_pdcp_written);
}

} // namespace winject

