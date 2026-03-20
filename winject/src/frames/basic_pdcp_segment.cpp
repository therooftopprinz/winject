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

#include "frames/basic_pdcp_segment.hpp"

template <bool IsConst>
basic_pdcp_segment_t<IsConst>::basic_pdcp_segment_t(byte_ptr base_, size_t size, bool has_sn_, bool has_offset_)
    : base(base_)
    , max_size(size)
    , has_sn(has_sn_)
    , has_offset(has_offset_)
{}

template <bool IsConst>
void basic_pdcp_segment_t<IsConst>::rebase(byte_ptr new_base, size_t new_size)
{
    base     = new_base;
    max_size = new_size;
    rescan();
}

template <bool IsConst>
typename basic_pdcp_segment_t<IsConst>::byte_ptr basic_pdcp_segment_t<IsConst>::get_base() const
{
    return base;
}

template <bool IsConst>
size_t basic_pdcp_segment_t<IsConst>::get_max_size() const
{
    return max_size;
}

template <bool IsConst>
void basic_pdcp_segment_t<IsConst>::reset()
{
    size    = nullptr;
    sn      = nullptr;
    offset  = nullptr;
    payload = nullptr;
}

template <bool IsConst>
bool basic_pdcp_segment_t<IsConst>::rescan()
{
    is_valid_ = true;
    auto start = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(base));
    safe_checker checker(start, start + max_size, is_valid_);

    size = checker.get<winject::BEU16UA>();
    if (!is_valid())
        return false;

    if (has_sn)
    {
        sn = checker.get<winject::BEU16UA>();
        if (!is_valid())
            return false;
    }

    if (has_offset)
    {
        offset = checker.get<winject::BEU16UA>();
        if (!is_valid())
            return false;
    }

    payload = checker.get_n(checker.remaining());
    return is_valid();
}

template <bool IsConst>
template <bool B, typename>
void basic_pdcp_segment_t<IsConst>::set_SN(pdcp_sn_t value)
{
    *sn = value;
}

template <bool IsConst>
pdcp_sn_t basic_pdcp_segment_t<IsConst>::get_SN() const
{
    return *sn;
}

template <bool IsConst>
template <bool B, typename>
void basic_pdcp_segment_t<IsConst>::set_OFFSET(pdcp_segment_offset_t value)
{
    *offset = value;
}

template <bool IsConst>
pdcp_segment_offset_t basic_pdcp_segment_t<IsConst>::get_OFFSET() const
{
    return *offset;
}

template <bool IsConst>
pdcp_segment_offset_t basic_pdcp_segment_t<IsConst>::get_SIZE() const
{
    return size ? (static_cast<pdcp_segment_offset_t>(*size) & 0x7FFF) : 0;
}

template <bool IsConst>
template <bool B, typename>
void basic_pdcp_segment_t<IsConst>::set_SIZE(pdcp_segment_offset_t size_)
{
    *size = (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) | size_;
}

template <bool IsConst>
template <bool B, typename>
void basic_pdcp_segment_t<IsConst>::set_LAST(bool is_last)
{
    *size = (uint16_t(is_last) << 15) | get_SIZE();
}

template <bool IsConst>
bool basic_pdcp_segment_t<IsConst>::is_LAST() const
{
    return (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) != 0;
}

template <bool IsConst>
size_t basic_pdcp_segment_t<IsConst>::get_header_size() const
{
    return sizeof(*size) + has_sn * sizeof(*sn) + has_offset * sizeof(*offset);
}

template <bool IsConst>
pdcp_segment_offset_t basic_pdcp_segment_t<IsConst>::get_payload_size() const
{
    return get_SIZE() - get_header_size();
}

template <bool IsConst>
typename basic_pdcp_segment_t<IsConst>::byte_ptr basic_pdcp_segment_t<IsConst>::get_payload() const
{
    return payload;
}

template <bool IsConst>
template <bool B, typename>
bool basic_pdcp_segment_t<IsConst>::set_payload_size(pdcp_segment_offset_t payload_size)
{
    auto new_size = get_header_size() + payload_size;
    set_SIZE(get_header_size() + payload_size);
    if (new_size > max_size)
    {
        is_valid_ = false;
    }
    return is_valid();
}

template <bool IsConst>
bool basic_pdcp_segment_t<IsConst>::is_header_valid() const
{
    return max_size >= get_header_size();
}

template <bool IsConst>
bool basic_pdcp_segment_t<IsConst>::is_valid()
{
    return is_valid_;
}

template class basic_pdcp_segment_t<false>;
template class basic_pdcp_segment_t<true>;

template void basic_pdcp_segment_t<false>::set_SN<false, void>(pdcp_sn_t);
template void basic_pdcp_segment_t<false>::set_OFFSET<false, void>(pdcp_segment_offset_t);
template void basic_pdcp_segment_t<false>::set_SIZE<false, void>(pdcp_segment_offset_t);
template void basic_pdcp_segment_t<false>::set_LAST<false, void>(bool);
template bool basic_pdcp_segment_t<false>::set_payload_size<false, void>(pdcp_segment_offset_t);
