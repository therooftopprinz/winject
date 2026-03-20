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

#ifndef __WINJECT_FRAMES_LLC_PAYLOAD_ACK_HPP__
#define __WINJECT_FRAMES_LLC_PAYLOAD_ACK_HPP__

#include "frames/frame_types.hpp"

// LLC-CTL-ACK Payload:
//    +-------------------------------+
// 00 | LLC SN                        |
//    +-------------------------------+
// 01 | COUNT                         |
//    +-------------------------------+

struct llc_payload_ack_t
{
    llc_sn_t sn;
    llc_sn_t count;
} __attribute((packed));

#endif // __WINJECT_FRAMES_LLC_PAYLOAD_ACK_HPP__
