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

#ifndef __PDCP_PDCP_UL_HPP__
#define __PDCP_PDCP_UL_HPP__

#include <bfc/buffer.hpp>
#include <frame_defs.hpp>

#include <deque>

namespace winject
{

struct pdcp_ul_config_t
{
    // @brief Breakdown frame data into smaller packets.
    bool allow_segmentation = false;
    // @brief Allow reordering of the packets.
    bool allow_reordering = false;
    // @brief Minimum size of the packet to be committed.
    size_t min_commit_size  = 7;
};

/* @brief PDCP UL class
 * This class is responsible for the PDCP Uplink Logic
 */
class pdcp_ul
{
public:
    enum status_code_e
    {
        STATUS_CODE_SUCCESS,
        STATUS_CODE_OUTPUT_EMPTY,
        STATUS_CODE_OUTPUT_LESS_THAN_MIN_COMMIT_SIZE,
        STATUS_CODE_NO_DATA,
        STATUS_CODE_NO_SEGMENTS_WRITTEN,
    };

    pdcp_ul(const pdcp_ul_config_t& config);
    ~pdcp_ul();

    void          reset();
    void          reconfigure(const pdcp_ul_config_t& config);
    pdcp_ul_config_t
                  get_config();
    bool          on_frame_data(bfc::buffer_view packet);
    size_t        get_outstanding_bytes();
    size_t        get_outstanding_packet();
    size_t        get_min_commit_size();
    ssize_t       write_pdcp(bfc::buffer_view buffer);
    status_code_e get_status();

private:
    pdcp_ul_config_t config;
    status_code_e status = STATUS_CODE_SUCCESS;
    std::deque<bfc::buffer> outstanding_buffers;
    size_t data_offset = 0;
    pdcp_sn_t sequence_number = 0;
    size_t total_in_bytes = 0;
    size_t total_out_bytes = 0;
};

} // namespace winject

#endif // __PDCP_PDCP_UL_HPP__
