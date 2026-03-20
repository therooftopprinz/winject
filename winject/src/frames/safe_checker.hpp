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

#ifndef __WINJECT_FRAMES_SAFE_CHECKER_HPP__
#define __WINJECT_FRAMES_SAFE_CHECKER_HPP__

#include <cstddef>
#include <cstdint>

class safe_checker
{
public:
    safe_checker() = delete;
    safe_checker(uint8_t* start, uint8_t* end, bool& validity)
        : start(start)
        , current(start)
        , end(end)
        , validity(validity)
    {}

    template <typename T>
    T* get()
    {
        if (current + sizeof(T) > end)
        {
            validity = false;
            return nullptr;
        }
        auto rv = reinterpret_cast<T*>(current);
        current += sizeof(T);
        return rv;
    }

    uint8_t* get_n(size_t n)
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

    bool is_valid() const
    {
        return validity;
    }

    operator bool() const
    {
        return is_valid();
    }

    size_t remaining() const
    {
        return size_t(end - current);
    }

private:
    uint8_t* start = nullptr;
    uint8_t* current = nullptr;
    uint8_t* end = nullptr;
    bool& validity;
};

#endif // __WINJECT_FRAMES_SAFE_CHECKER_HPP__
