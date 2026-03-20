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

#ifndef __WINJECT_FRAMES_BASIC_PDCP_SEGMENT_T_HPP__
#define __WINJECT_FRAMES_BASIC_PDCP_SEGMENT_T_HPP__

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "frames/frame_types.hpp"
#include "frames/safe_checker.hpp"
#include "safeint.hpp"

//     +---+---------------------------+
//  00 | L | SIZE                      | 02
//     +---+---------------------------|
//     | PACKET SN (OPTIONAL)          | 02
//     +-------------------------------+
//     | OFFSET (OPTIONAL)             | 02
//     +-------------------------------+
//     | PAYLOAD                       | EOS
//     +-------------------------------+

// @brief PDCP segment frame definition
//        Handles set and get for PDCP Segment
// @note UB when accessing fields without proper configuration.
//       You must check is_valid() after rescan() or set_SIZE() to ensure the segment is valid.
template <bool IsConst>
class basic_pdcp_segment_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;
    using u16_ptr  = std::conditional_t<IsConst, const winject::BEU16UA*, winject::BEU16UA*>;

    basic_pdcp_segment_t() = delete;
    basic_pdcp_segment_t(byte_ptr base, size_t size, bool has_sn = false, bool has_offset = false);

    void     rebase(byte_ptr new_base, size_t new_size);
    byte_ptr get_base() const;
    size_t   get_max_size() const;
    void     reset();
    bool     rescan();

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SN(pdcp_sn_t value);
    pdcp_sn_t get_SN() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_OFFSET(pdcp_segment_offset_t value);
    pdcp_segment_offset_t get_OFFSET() const;

    pdcp_segment_offset_t get_SIZE() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SIZE(pdcp_segment_offset_t size_);

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_LAST(bool is_last);

    bool is_LAST() const;

    size_t get_header_size() const;
    pdcp_segment_offset_t get_payload_size() const;
    byte_ptr get_payload() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    bool set_payload_size(pdcp_segment_offset_t payload_size);

    bool is_header_valid() const;
    bool is_valid();

private:
    byte_ptr base       = nullptr;
    size_t   max_size   = 0;
    u16_ptr  size       = nullptr;
    bool     has_sn     = false;
    u16_ptr  sn         = nullptr;
    bool     has_offset = false;
    u16_ptr  offset     = nullptr;
    byte_ptr payload    = nullptr;
    bool     is_valid_  = false;
};

using pdcp_segment_t       = basic_pdcp_segment_t<false>;
using pdcp_segment_const_t = basic_pdcp_segment_t<true>;

#endif // __WINJECT_FRAMES_BASIC_PDCP_SEGMENT_T_HPP__
