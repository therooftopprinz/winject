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

/**
 * @file pdcp_dl.hpp
 * @brief PDCP downlink reassembly/ordering logic.
 */

#include <bfc/buffer.hpp>
#include <bfc/sized_buffer.hpp>
#include "frame_defs.hpp"

#include <deque>
#include <unordered_set>
#include <vector>

namespace winject
{

/**
 * @brief PDCP downlink configuration.
 */
struct pdcp_dl_config_t
{
    /** @brief Allow segmentation input to be reassembled into larger packets. */
    bool allow_segmentation = false;
    /** @brief Allow reordering by using PDCP SN and emitting in-order. */
    bool allow_reordering = false;
    /**
     * @brief Length of the reordering ring (slots).
     *
     * When a ring slot is occupied by a different SN, segments are stored in
     * an overflow deque instead.
     */
    size_t reorder_size = 512;
};

/**
 * @brief PDCP downlink logic with circular-buffer reordering.
 *
 * Collects PDCP segments from `on_pdcp_data()`, reassembles frames, and
 * optionally reorders them before they become available via `pop()`.
 */
class pdcp_dl
{
public:
    /**
     * @brief Status code for PDCP downlink processing.
     *
     * Returned through `get_status()` and set by `on_pdcp_data()` on error/success.
     */
    enum status_code_e
    {
        /** @brief Processing succeeded. */
        STATUS_CODE_SUCCESS,
        /** @brief Input PDCP buffer was empty. */
        STATUS_CODE_INPUT_EMPTY,
        /** @brief Segment header scan/validation failed. */
        STATUS_CODE_SEGMENT_SCAN_FAILED,
        /** @brief Segment length exceeded the remaining input. */
        STATUS_CODE_SEGMENT_OUTSIDE_INPUT,
        /** @brief Segment reserved/invalid payload size used. */
        STATUS_CODE_RESERVED_VALUE_USED
    };

    /**
     * @brief Construct PDCP downlink logic from configuration.
     * @param config PDCP downlink configuration.
     */
    pdcp_dl(const pdcp_dl_config_t& config);
    
    /**
     * @brief Destroy PDCP downlink logic.
     */
    ~pdcp_dl();

    /** @brief Reset internal state and clear outstanding buffers/frames. */
    void reset();
    
    /**
     * @brief Apply a new configuration and re-initialize internal state.
     * @param config New PDCP downlink configuration.
     */
    void reconfigure(const pdcp_dl_config_t& config);
    
    /**
     * @brief Get the currently active configuration.
     * @return Active configuration (by value).
     */
    pdcp_dl_config_t get_config();
    
    /**
     * @brief Consume a PDCP downlink byte stream and build reassembled frames.
     *
     * Segments are validated during parsing; on error `status` is updated and the
     * function returns `false`.
     *
     * When `config.allow_reordering` is enabled, frames are emitted in-order and
     * stored for later retrieval by `pop()`.
     *
     * @param pdcp Input PDCP bytes.
     * @return `true` on success; `false` on parse/reassembly errors.
     */
    bool on_pdcp_data(bfc::const_buffer_view pdcp);
    
    /**
     * @brief Get total number of outstanding bytes ready to be popped.
     * @return Outstanding bytes (bytes in reassembled frames not yet popped).
     */
    size_t get_outstanding_bytes();
    
    /**
     * @brief Get number of outstanding completed frames ready to be popped.
     * @return Completed frame count.
     */
    size_t get_outstanding_packet();
    
    /**
     * @brief Pop next completed frame (FIFO).
     * @return Next completed frame; returns an empty buffer when none is available.
     */
    bfc::sized_buffer pop();
    
    /** @brief Get the last status set by `on_pdcp_data()`. */
    status_code_e get_status();

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
