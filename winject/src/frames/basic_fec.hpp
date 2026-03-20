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

#ifndef __WINJECT_FRAMES_BASIC_FEC_T_HPP__
#define __WINJECT_FRAMES_BASIC_FEC_T_HPP__

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "frames/frame_types.hpp"
#include "frames/safe_checker.hpp"
#include "info_defs.hpp"

//     +-------------------------------+
//     | FEC_TYPE                      | 01
//     +-------------------------------+
//     | N (FEC_TYPE != 0)             | 01 * (FEC_TYPE!=0)
//     +-------------------------------+
//     | REDUNDANCY                    | FEC_R_SZ * N
//     +-------------------------------+
//     | DATA                          | FEC_D_SZ * N
//     +-------------------------------+

template <bool IsConst>
class basic_fec_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;

    basic_fec_t(byte_ptr base, size_t size);

    void     reset();
    bool     is_valid() const;
    byte_ptr get_base() const;

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void     init(uint8_t fec, uint8_t n_);
    void     rescan();
    uint8_t  get_FEC_TYPE() const;
    uint8_t  get_N() const;
    byte_ptr get_data_blocks() const;
    byte_ptr get_redu_blocks() const;
    size_t   get_fec_d_sz() const;
    size_t   get_fec_r_sz() const;
    size_t   get_data_sz() const;
    size_t   get_redu_sz() const;

private:
    byte_ptr base         = nullptr;
    llc_sz_t max_size     = 0;
    size_t   fec_d_sz     = 0;
    size_t   fec_r_sz     = 0;

    byte_ptr fec_type     = nullptr;
    byte_ptr n            = nullptr;
    byte_ptr data_blocks  = nullptr;
    byte_ptr redu_blocks  = nullptr;
    bool     is_valid_    = false;
};

extern template class basic_fec_t<false>;
extern template class basic_fec_t<true>;
extern template void basic_fec_t<false>::init<false>(uint8_t, uint8_t);

using fec_t       = basic_fec_t<false>;
using fec_const_t = basic_fec_t<true>;

#endif // __WINJECT_FRAMES_BASIC_FEC_T_HPP__
