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

#include "frames/basic_llc.hpp"

#include <cstring>

template <bool IsConst>
basic_llc_t<IsConst>::basic_llc_t(size_t crc_size)
    : crc_size(crc_size)
{}

template <bool IsConst>
basic_llc_t<IsConst>::basic_llc_t(byte_ptr base, size_t size, size_t crc_size)
    : base(base)
    , max_size(size)
    , crc_size(crc_size)
{}

template <bool IsConst>
basic_llc_t<IsConst>::basic_llc_t(const basic_llc_t& other)
    : base(other.base)
    , max_size(other.max_size)
    , crc_size(other.crc_size)
{}

template <bool IsConst>
basic_llc_t<IsConst>& basic_llc_t<IsConst>::operator=(const basic_llc_t& other)
{
    base = other.base;
    max_size = other.max_size;
    return *this;
}

template <bool IsConst>
void basic_llc_t<IsConst>::rebase(byte_ptr new_base, size_t new_max_size)
{
    base = new_base;
    max_size = new_max_size;
}

template <bool IsConst>
typename basic_llc_t<IsConst>::byte_ptr basic_llc_t<IsConst>::get_base() const
{
    return base;
}

template <bool IsConst>
size_t basic_llc_t<IsConst>::get_max_size() const
{
    return max_size;
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_SN(llc_sn_t sn)
{
    set(0, mask_SN, shift_SN, sn);
}

template <bool IsConst>
llc_sn_t basic_llc_t<IsConst>::get_SN() const
{
    return get(0, mask_SN, shift_SN);
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_LCID(lcid_t lcid)
{
    set(1, mask_LCID, shift_LCID, lcid);
}

template <bool IsConst>
lcid_t basic_llc_t<IsConst>::get_LCID() const
{
    return get(1, mask_LCID, shift_LCID);
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_A(bool val)
{
    set(1, mask_A, shift_A, val);
}

template <bool IsConst>
bool basic_llc_t<IsConst>::get_A() const
{
    return get(1, mask_A, shift_A);
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_SIZE(llc_sz_t size)
{
    set(1, mask_SIZEH, shift_SIZEH, size >> 8);
    set(2, mask_SIZEL, shift_SIZEL, size);
}

template <bool IsConst>
llc_sz_t basic_llc_t<IsConst>::get_SIZE() const
{
    return (get(1, mask_SIZEH, shift_SIZEH) << 8) |
           (get(2, mask_SIZEL, shift_SIZEL));
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_CRC(byte_ptr crc)
{
    std::memcpy(base + 3, crc, crc_size);
}

template <bool IsConst>
typename basic_llc_t<IsConst>::byte_ptr basic_llc_t<IsConst>::get_CRC() const
{
    return base + 3;
}

template <bool IsConst>
llc_sz_t basic_llc_t<IsConst>::get_header_size() const
{
    return 3 + crc_size;
}

template <bool IsConst>
typename basic_llc_t<IsConst>::byte_ptr basic_llc_t<IsConst>::get_payload() const
{
    return base + get_header_size();
}

template <bool IsConst>
llc_sz_t basic_llc_t<IsConst>::get_max_payload_size() const
{
    return max_size - get_header_size();
}

template <bool IsConst>
llc_sz_t basic_llc_t<IsConst>::get_payload_size() const
{
    return get_SIZE() - get_header_size();
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set_payload_size(llc_sz_t size)
{
    set_SIZE(size + get_header_size());
}

template <bool IsConst>
bool basic_llc_t<IsConst>::is_valid() const
{
    return max_size >= get_SIZE();
}

template <bool IsConst>
bool basic_llc_t<IsConst>::is_header_valid() const
{
    return max_size >= get_header_size();
}

template <bool IsConst>
uint8_t basic_llc_t<IsConst>::get(int index, uint8_t mask, uint8_t shift) const
{
    return (base[index] & mask) >> shift;
}

template <bool IsConst>
template <bool B, typename>
void basic_llc_t<IsConst>::set(int index, uint8_t mask, uint8_t shift, uint8_t val)
{
    uint8_t mv = base[index] & ~mask;
    mv |= ((val << shift) & mask);
    base[index] = mv;
}

template class basic_llc_t<false>;
template class basic_llc_t<true>;

template void basic_llc_t<false>::set_SN<false, void>(llc_sn_t);
template void basic_llc_t<false>::set_LCID<false, void>(lcid_t);
template void basic_llc_t<false>::set_A<false, void>(bool);
template void basic_llc_t<false>::set_SIZE<false, void>(llc_sz_t);
template void basic_llc_t<false>::set_CRC<false, void>(byte_ptr);
template void basic_llc_t<false>::set_payload_size<false, void>(llc_sz_t);
template void basic_llc_t<false>::set<false, void>(int, uint8_t, uint8_t, uint8_t);
