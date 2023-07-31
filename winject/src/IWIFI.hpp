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

#ifndef __WINJECTUM_IWIFI_HPP__
#define __WINJECTUM_IWIFI_HPP__

#include <functional>
#include <chrono>

struct IWIFI
{
    virtual ssize_t send(const uint8_t*, size_t) = 0;
    virtual ssize_t recv(uint8_t*, size_t) = 0;
    virtual int handle() = 0;
};

#endif // __WINJECTUM_IWIFI_HPP__
