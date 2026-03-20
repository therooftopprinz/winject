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

/**
 * @file pdcp_ul.hpp
 * @brief PDCP uplink segmentation/packetization logic.
 */

#include <bfc/buffer.hpp>
#include <frame_defs.hpp>

#include <deque>

namespace winject
{

/**
 * @brief PDCP uplink configuration.
 */
struct pdcp_ul_config_t
{
    /** @brief Allow segmentation of larger PDCP SDUs into smaller packets. */
    bool allow_segmentation = false;
    /** @brief Allow reordering by setting SN/OFFSET fields on generated segments. */
    bool allow_reordering = false;
    /**
     * @brief Minimum size of the packet to be committed.
     *
     * When `write_pdcp()` is given a buffer smaller than this value, it fails
     * with `STATUS_CODE_OUTPUT_LESS_THAN_MIN_COMMIT_SIZE`.
     */
    size_t min_commit_size  = 7;
};

/**
 * @brief PDCP uplink logic.
 *
 * Buffers incoming PDCP frame data through `on_frame_data()` and later
 * segments/encodes it into PDCP packets through `write_pdcp()`.
 */
class pdcp_ul
{
public:
    /**
     * @brief Status code for PDCP uplink processing.
     *
     * Returned through `get_status()` and set by `write_pdcp()` on error/success.
     */
    enum status_code_e
    {
        /** @brief Processing succeeded. */
        STATUS_CODE_SUCCESS,
        /** @brief `write_pdcp()` was called with an empty output buffer. */
        STATUS_CODE_OUTPUT_EMPTY,
        /** @brief Output buffer is smaller than `config.min_commit_size`. */
        STATUS_CODE_OUTPUT_LESS_THAN_MIN_COMMIT_SIZE,
        /** @brief No input data available (no segments to write). */
        STATUS_CODE_NO_DATA,
        /** @brief No segments were written (e.g. due to header/segment limits). */
        STATUS_CODE_NO_SEGMENTS_WRITTEN,
    };

    /**
     * @brief Construct PDCP uplink logic from configuration.
     * @param config PDCP uplink configuration.
     */
    pdcp_ul(const pdcp_ul_config_t& config);
    
    /**
     * @brief Destroy PDCP uplink logic.
     */
    ~pdcp_ul();

    /** @brief Reset internal state and clear buffered input. */
    void reset();
    /**
     * @brief Apply a new configuration and keep internal state consistent.
     * @param config New PDCP uplink configuration.
     */
    void reconfigure(const pdcp_ul_config_t& config);
    
    /**
     * @brief Get the currently active configuration.
     * @return Active configuration (by value).
     */
    pdcp_ul_config_t get_config();
    
    /**
     * @brief Buffer incoming frame data for later PDCP segmentation.
     * @param packet Input frame bytes.
     * @return `true` when bytes were accepted; `false` when `packet` is empty.
     *
     * Note: this function does not update `status`; it is updated by `write_pdcp()`.
     */
    bool on_frame_data(bfc::buffer_view packet);
    
    /**
     * @brief Get total number of bytes currently buffered for output.
     * @return Outstanding (buffered minus consumed) bytes.
     */
    size_t get_outstanding_bytes();
    
    /**
     * @brief Get number of buffered input frames waiting to be segmented.
     * @return Outstanding frame count.
     */
    size_t get_outstanding_packet();
    
    /** @brief Get configured minimum commit size. */
    size_t get_min_commit_size();

    /**
     * @brief Write/encode PDCP segments into the provided output buffer.
     * @param buffer Output buffer view.
     * @return Number of bytes written on success; `-1` on failure.
     *
     * On failure, `status` is updated and can be queried with `get_status()`.
     */
    ssize_t write_pdcp(bfc::buffer_view buffer);
    
    /**
     * @brief Get the last status set by `write_pdcp()`.
     * @return Current `status`.
     */
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
