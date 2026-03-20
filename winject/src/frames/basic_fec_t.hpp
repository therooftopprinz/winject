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

    basic_fec_t(byte_ptr base, size_t size)
        : base(base)
        , max_size(size)
    {}

    void reset()
    {
        fec_type    = nullptr;
        n           = nullptr;
        data_blocks = nullptr;
        redu_blocks = nullptr;
        fec_d_sz    = 0;
        fec_r_sz    = 0;
        is_valid_   = false;
    }

    bool is_valid() const
    {
        return is_valid_;
    }

    byte_ptr get_base() const
    {
        return base;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void init(uint8_t fec, uint8_t n_)
    {
        is_valid_ = true;
        safe_checker checker(base, base + max_size, is_valid_);
        fec_type = checker.get<uint8_t>();
        if (!is_valid()) return;
        *fec_type = fec;

        rescan();

        if (n)
        {
            *n = n_;
        }
    }

    void rescan()
    {
        is_valid_ = true;
        auto start = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(base));
        safe_checker checker(start, start + max_size, is_valid_);
        fec_type = checker.get<uint8_t>();
        if (!is_valid()) return reset();

        auto ft = (fec_type_e)(*fec_type);
        if (ft != E_FEC_TYPE_NONE)
        {
            n = checker.get<uint8_t>();
            if (!is_valid()) return;
        }

        if (E_FEC_TYPE_RS_255_247 == ft)
        {
            fec_d_sz = 247;
            fec_r_sz = 255 - 247;
        }
        else if (E_FEC_TYPE_RS_255_239 == ft)
        {
            fec_d_sz = 239;
            fec_r_sz = 255 - 239;
        }
        else if (E_FEC_TYPE_RS_255_223 == ft)
        {
            fec_d_sz = 223;
            fec_r_sz = 255 - 223;
        }
        else if (E_FEC_TYPE_RS_255_191 == ft)
        {
            fec_d_sz = 191;
            fec_r_sz = 255 - 191;
        }
        else if (E_FEC_TYPE_RS_255_127 == ft)
        {
            fec_d_sz = 127;
            fec_r_sz = 255 - 127;
        }

        redu_blocks = checker.get_n(*n * fec_r_sz);
        if (!is_valid()) return;
        data_blocks = checker.get_n(*n * fec_d_sz);
        if (!is_valid()) return;
    }

    uint8_t get_FEC_TYPE() const
    {
        return *fec_type;
    }

    uint8_t get_N() const
    {
        return *n;
    }

    byte_ptr get_data_blocks() const
    {
        return data_blocks;
    }

    byte_ptr get_redu_blocks() const
    {
        return redu_blocks;
    }

    size_t get_fec_d_sz() const
    {
        return fec_d_sz;
    }

    size_t get_fec_r_sz() const
    {
        return fec_r_sz;
    }

    size_t get_data_sz() const
    {
        return get_N() * get_fec_d_sz();
    }

    size_t get_redu_sz() const
    {
        return get_N() * get_fec_r_sz();
    }

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

using fec_t       = basic_fec_t<false>;
using fec_const_t = basic_fec_t<true>;

#endif // __WINJECT_FRAMES_BASIC_FEC_T_HPP__
