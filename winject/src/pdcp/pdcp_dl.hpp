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

#ifndef __PDCP_PDCP_DL_CB_HPP__
#define __PDCP_PDCP_DL_CB_HPP__

#include <bfc/buffer.hpp>
#include <bfc/sized_buffer.hpp>
#include "frame_defs.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace winject
{

struct pdcp_dl_config_t
{
    // @brief Breakdown frame data into smaller packets.
    bool allow_segmentation = false;
    // @brief Allow reordering of the packets.
    bool allow_reordering = false;
    // @brief Length of reorder buffer (slots). When slot is occupied by another SN, frame goes to overflow deque.
    size_t reorder_size = 512;
};

/* @brief PDCP DL class with circular-buffer reordering
 * Same interface as pdcp_dl but uses a circular buffer for reordering instead of a map.
 * When the circular buffer slot is occupied (different SN), frames are stored in an overflow deque.
 */
class pdcp_dl
{
public:
    enum status_code_e
    {
        STATUS_CODE_SUCCESS,
        STATUS_CODE_INPUT_EMPTY,
        STATUS_CODE_SEGMENT_SCAN_FAILED,
        STATUS_CODE_SEGMENT_OUTSIDE_INPUT,
        STATUS_CODE_RESERVED_VALUE_USED
    };

    pdcp_dl(const pdcp_dl_config_t& config);
    ~pdcp_dl();

    void                reset();
    void                reconfigure(const pdcp_dl_config_t& config);
    pdcp_dl_config_t get_config();
    bool                on_pdcp_data(bfc::const_buffer_view pdcp);
    size_t              get_outstanding_bytes();
    size_t              get_outstanding_packet();
    bfc::sized_buffer   pop();
    status_code_e       get_status();

private:
    struct reassembly_state_t
    {
        bfc::sized_buffer buffer;
        size_t accumulated_size = 0;
        size_t expected_size = 0;
        std::unordered_set<size_t> recvd_offsets;
        bool has_expected_size = false;
        bool is_complete = false;
    };

    struct slot_t
    {
        pdcp_sn_t sn = 0;
        reassembly_state_t state;
        bool valid = false;
    };

    void flush_completed_in_order();
    size_t slot_index(pdcp_sn_t sn) const;
    reassembly_state_t* get_or_put_slot(pdcp_sn_t sn);
    reassembly_state_t* find_overflow(pdcp_sn_t sn);

    pdcp_dl_config_t config;
    status_code_e status = STATUS_CODE_SUCCESS;

    std::vector<slot_t> reorder_ring;
    std::deque<std::pair<pdcp_sn_t, reassembly_state_t>> overflow_deque;

    pdcp_sn_t next_expected_sn = 0;
    std::deque<bfc::sized_buffer> completed_frames;
    size_t outstanding_bytes = 0;
};

} // namespace winject

#endif // __PDCP_PDCP_DL_CB_HPP__
