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

#ifndef __WINJECTUM_INFO_DEPS_HPP__
#define __WINJECTUM_INFO_DEPS_HPP__

#include <cstddef>
#include <cstdint>

#include "pdu.hpp"

enum fec_type_e
{
    E_FEC_TYPE_NONE,
    E_FEC_TYPE_RS_255_247,
    E_FEC_TYPE_RS_255_239,
    E_FEC_TYPE_RS_255_223,
    E_FEC_TYPE_RS_255_191,
    E_FEC_TYPE_RS_255_127
};

struct frame_info_t
{
    size_t slot_number = 0;
    fec_type_e fec_type = E_FEC_TYPE_NONE;
    uint64_t slot_interval_us = 0;
    size_t frame_payload_size = 0;
};

class pdu_t;

struct tx_info_t
{
    const frame_info_t& in_frame_info;
    // used to inform LLC that out_pdu is allowed for data allocation
    bool in_allow_data = true;
    // used to inform scheduler that data is available
    bool out_tx_available = false;
    // used to inform scheduler data has already allocated in this slot
    bool out_has_data_loaded = false;
    pdu_t out_pdu;
    size_t out_allocated = 0;
};

struct rx_info_t
{
    pdu_t in_pdu;
};

#endif // __WINJECTUM_INFO_DEPS_HPP__
