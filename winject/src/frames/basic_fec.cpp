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

#include "frames/basic_fec.hpp"

template <bool IsConst>
basic_fec_t<IsConst>::basic_fec_t(byte_ptr base, size_t size)
    : base(base)
    , max_size(size)
{}

template <bool IsConst>
void basic_fec_t<IsConst>::reset()
{
    fec_type    = nullptr;
    n           = nullptr;
    data_blocks = nullptr;
    redu_blocks = nullptr;
    fec_d_sz    = 0;
    fec_r_sz    = 0;
    is_valid_   = false;
}

template <bool IsConst>
bool basic_fec_t<IsConst>::is_valid() const
{
    return is_valid_;
}

template <bool IsConst>
typename basic_fec_t<IsConst>::byte_ptr basic_fec_t<IsConst>::get_base() const
{
    return base;
}

template <bool IsConst>
template <bool B, typename>
void basic_fec_t<IsConst>::init(uint8_t fec, uint8_t n_)
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

template <bool IsConst>
void basic_fec_t<IsConst>::rescan()
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

template <bool IsConst>
uint8_t basic_fec_t<IsConst>::get_FEC_TYPE() const
{
    return *fec_type;
}

template <bool IsConst>
uint8_t basic_fec_t<IsConst>::get_N() const
{
    return *n;
}

template <bool IsConst>
typename basic_fec_t<IsConst>::byte_ptr basic_fec_t<IsConst>::get_data_blocks() const
{
    return data_blocks;
}

template <bool IsConst>
typename basic_fec_t<IsConst>::byte_ptr basic_fec_t<IsConst>::get_redu_blocks() const
{
    return redu_blocks;
}

template <bool IsConst>
size_t basic_fec_t<IsConst>::get_fec_d_sz() const
{
    return fec_d_sz;
}

template <bool IsConst>
size_t basic_fec_t<IsConst>::get_fec_r_sz() const
{
    return fec_r_sz;
}

template <bool IsConst>
size_t basic_fec_t<IsConst>::get_data_sz() const
{
    return get_N() * get_fec_d_sz();
}

template <bool IsConst>
size_t basic_fec_t<IsConst>::get_redu_sz() const
{
    return get_N() * get_fec_r_sz();
}

template class basic_fec_t<false>;
template class basic_fec_t<true>;
template void basic_fec_t<false>::init<false>(uint8_t, uint8_t);
