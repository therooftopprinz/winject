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

#ifndef __WINJECT_FRAMES_BASIC_LLC_T_HPP__
#define __WINJECT_FRAMES_BASIC_LLC_T_HPP__

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "frames/frame_types.hpp"

//        +-------------------------------+
//     00 | LLC SN                        |
//        +---------------+---+-----------+
//     01 | LCID          | A | SIZEH     |
//        +---------------+---+-----------+
//     02 | SIZEL                         |
//        +-------------------------------+
//     03 | CRC                           | CRCSZ
//        +-------------------------------+
// HDR_SZ | Payload                       |
//        +-------------------------------+

template <bool IsConst>
class basic_llc_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;


    basic_llc_t() = delete;

    basic_llc_t(byte_ptr base, size_t size, size_t crc_size = 0);

    basic_llc_t(const basic_llc_t& other);

    basic_llc_t& operator=(const basic_llc_t& other);

    void rebase(byte_ptr new_base, size_t new_max_size);

    byte_ptr get_base() const;

    size_t get_max_size() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SN(llc_sn_t sn);

    llc_sn_t get_SN() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_LCID(lcid_t lcid);

    lcid_t get_LCID() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_A(bool val);

    bool get_A() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SIZE(llc_sz_t size);

    llc_sz_t get_SIZE() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_CRC(byte_ptr crc);

    byte_ptr get_CRC() const;

    llc_sz_t get_header_size() const;

    byte_ptr get_payload() const;

    llc_sz_t get_max_payload_size() const;

    llc_sz_t get_payload_size() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_payload_size(llc_sz_t size);

    bool is_valid() const;

    bool is_header_valid() const;

private:
    static constexpr uint8_t mask_SN    = 0b11111111;
    static constexpr uint8_t mask_LCID  = 0b11110000;
    static constexpr uint8_t mask_A     = 0b00001000;
    static constexpr uint8_t mask_SIZEH = 0b00000111;
    static constexpr uint8_t mask_SIZEL = 0b11111111;

    static constexpr uint8_t shift_SN    = 0;
    static constexpr uint8_t shift_LCID  = 4;
    static constexpr uint8_t shift_A     = 3;
    static constexpr uint8_t shift_SIZEH = 0;
    static constexpr uint8_t shift_SIZEL = 0;

    uint8_t get(int index, uint8_t mask, uint8_t shift) const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set(int index, uint8_t mask, uint8_t shift, uint8_t val);

    byte_ptr     base     = nullptr;
    llc_sz_t     max_size = 0;
    const size_t crc_size = 0;
};

using llc_t       = basic_llc_t<false>;
using llc_const_t = basic_llc_t<true>;

#endif // __WINJECT_FRAMES_BASIC_LLC_T_HPP__
