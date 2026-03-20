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

#ifndef __WINJECT_FRAMES_FRAME_TYPES_HPP__
#define __WINJECT_FRAMES_FRAME_TYPES_HPP__

#include <cstddef>
#include <cstdint>

using llc_sn_t              = uint8_t;
using lcid_t                = uint8_t;
using llc_sz_t              = uint16_t;
using pdcp_sn_t             = uint16_t;
using pdcp_segment_offset_t = uint16_t;

constexpr llc_sn_t llc_sn_mask = 0b11111111;
constexpr size_t   llc_sn_size = llc_sn_mask + 1;

#endif // __WINJECT_FRAMES_FRAME_TYPES_HPP__
