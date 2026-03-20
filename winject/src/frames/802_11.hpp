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

#ifndef __WINJECT_802_11_hpp__
#define __WINJECT_802_11_hpp__

#include <cstddef>
#include <cstdint>
#include <string>

#include "../safeint.hpp"

namespace winject
{
namespace ieee_802_11
{

struct frame_control_t
{
    enum proto_type_e : uint8_t
    {
        E_PROTO_MASK                     = 0b00000011,
        E_PROTO_V0                       = 0b00000000,

        E_MTYPE_MASK                     = 0b00001100,
        E_MTYPE_MGNT                     = 0b00000000,
        E_MTYPE_CTRL                     = 0b00000100,
        E_MTYPE_DATA                     = 0b00001000,
        E_MTYPE_EXTN                     = 0b00001100,

        E_TYPE_MASK                      = 0b11111100,

        E_TYPE_ASSOCIATION_REQUEST       = 0b00000000,
        E_TYPE_ASSOCIATION_RESPONSE      = 0b00010000,
        E_TYPE_REASSOCIATION_REQUEST     = 0b00100000,
        E_TYPE_REASSOCIATION_RESPONSE    = 0b00110000,
        E_TYPE_PROBE_REQUEST             = 0b01000000,
        E_TYPE_PROBE_RESPONSE            = 0b01010000,
        E_TYPE_TIMING_ADVERTISEMENT      = 0b01100000,
        E_TYPE_BEACON                    = 0b10000000,
        E_TYPE_ATIM                      = 0b10010000,
        E_TYPE_DISASSOCIATION            = 0b10100000,
        E_TYPE_AUTHENTICATION            = 0b10110000,
        E_TYPE_DEAUTHENTICATION          = 0b11000000,
        E_TYPE_ACTION                    = 0b11010000,
        E_TYPE_ACTION_NACK               = 0b11100000,

        E_TYPE_TRIGGER3                  = 0b00100100,
        E_TYPE_TACK                      = 0b00110100,
        E_TYPE_BEAMFORMING_REPORT_POLL   = 0b01000100,
        E_TYPE_VHT_HE_NDP_ANNOUNCEMENT   = 0b01010100,
        E_TYPE_CONTROL_FRAME_EXTENSION   = 0b01100100,
        E_TYPE_CONTROL_WRAPPER           = 0b01110100,
        E_TYPE_BLOCK_ACK_REQUEST         = 0b10000100,
        E_TYPE_BLOCK_ACK                 = 0b10010100,
        E_TYPE_PS_POLL                   = 0b10100100,
        E_TYPE_RTS                       = 0b10110100,
        E_TYPE_CTS                       = 0b11000100,
        E_TYPE_ACK                       = 0b11010100,
        E_TYPE_CF_END                    = 0b11100100,
        E_TYPE_CF_END_CF_ACK             = 0b11110100,

        E_TYPE_DATA                      = 0b00001000,
        E_TYPE_NULL                      = 0b01001000,
        E_TYPE_QOS_DATA                  = 0b10001000,
        E_TYPE_QOS_DATA_CF_ACK           = 0b10011000,
        E_TYPE_QOS_DATA_CF_POLL          = 0b10101000,
        E_TYPE_QOS_DATA_CF_ACK_CF_POLL   = 0b10111000,
        E_TYPE_QOS_NULL                  = 0b11001000,
        E_TYPE_QOS_CF_POL                = 0b11101000,
        E_TYPE_QOS_CF_ACK_CF_POLL        = 0b11111000,
        E_TYPE_DMG_BEACON                = 0b00001100,
        E_TYPE_S1G_BEACON                = 0b00011100
    };

    enum flags_e : uint8_t
    {
        E_FLAGS_TO_DS      = 0b00000001,
        E_FLAGS_FROM_DS    = 0b00000010,
        E_FLAGS_MORE_FRAG  = 0b00000100,
        E_FLAGS_MORE_RETRY = 0b00001000,
        E_FLAGS_POWER_MGNT = 0b00010000,
        E_FLAGS_MORE_DATA  = 0b00100000,
        E_FLAGS_WEP        = 0b01000000,
        E_FLAGS_ORDER      = 0b10000000,
    };

    uint8_t protocol_type;
    uint8_t flags;
} __attribute__((__packed__));

struct address_t
{
    uint8_t address[6];

    uint64_t get();
    void set(uint64_t value);
} __attribute__((__packed__));

struct seq_ctl_t
{
    // @todo : not working in big endian
    uint16_t frag_sec;

    uint8_t get_fragment_num();
    void set_fragment_num(uint8_t value);

    uint16_t get_seq_num();
    void set_seq_num(uint16_t value);

} __attribute__((__packed__));

class frame_t
{
public:
    frame_t() = default;
    frame_t(uint8_t* buffer, uint8_t* last);

    void reset();

    void rescan();

    uint8_t* end();

    size_t size();

    size_t frame_body_size();

    void set_enable_fcs(bool val);

    void set_body_size(uint16_t size);

    frame_control_t* frame_control = nullptr;
    LEU16UA* duration = nullptr;
    address_t* address1 = nullptr;
    address_t* address2 = nullptr;
    address_t* address3 = nullptr;
    seq_ctl_t* seq_ctl = nullptr;
    address_t* address4 = nullptr;
    uint8_t* frame_body = nullptr;
    LEU32UA* fcs = nullptr;
    uint8_t* last = nullptr;
    bool is_enabled_fcs = true;
};

std::string to_string(frame_t& frame80211);

} // namespace 802_11
} // namespace winject

#endif // __WINJECT_802_11_hpp__
