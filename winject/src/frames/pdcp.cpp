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

#include "frames/pdcp.hpp"

pdcp_t::pdcp_t(uint8_t* base_, size_t size)
    : base(base_)
    , max_size(size)
{}

void pdcp_t::rescan()
{
    uint8_t* ptr = base;

    if (iv_size)
    {
        iv = ptr;
        ptr += iv_size;
    }

    if (hmac_size)
    {
        hmac = ptr;
        ptr += hmac_size;
    }

    payload = ptr;
}

size_t pdcp_t::get_header_size()
{
    return (hmac_size + iv_size);
}

bool pdcp_t::is_header_valid()
{
    return max_size > get_header_size();
}
