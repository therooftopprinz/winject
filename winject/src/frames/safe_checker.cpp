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

#include "frames/safe_checker.hpp"

safe_checker::safe_checker(uint8_t* start_, uint8_t* end_, bool& validity_)
    : start(start_)
    , current(start_)
    , end(end_)
    , validity(validity_)
{}

uint8_t* safe_checker::get_n(size_t n)
{
    if (current + n > end)
    {
        validity = false;
        return nullptr;
    }
    auto rv = reinterpret_cast<uint8_t*>(current);
    current += n;
    return rv;
}

bool safe_checker::is_valid() const
{
    return validity;
}

safe_checker::operator bool() const
{
    return is_valid();
}

size_t safe_checker::remaining() const
{
    return size_t(end - current);
}
