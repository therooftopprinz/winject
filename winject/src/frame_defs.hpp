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

#ifndef __WINJECTUM_FRAME_DEFS_HPP__
#define __WINJECTUM_FRAME_DEFS_HPP__

#include <cstdint>
#include <cstddef>
#include <optional>
#include "safeint.hpp"
#include "info_defs.hpp"

using llc_sn_t = uint8_t;
using lcid_t = uint8_t;
using llc_sz_t = uint16_t;
using pdcp_sn_t = uint16_t;
using pdcp_segment_offset_t = uint16_t;

//     +-------------------------------+
//  01 | FEC_TYPE                      | 01
//     +-------------------------------+
//  02 | N (FEC_TYPE != 0)             | 01 * (FEC_TYPE!=0)
//     +-------------------------------+
//  03 | REDUNDANCY                    | FEC_R_SZ * N
//     +-------------------------------+
//     | DATA                          | FEC_D_SZ * N
//     +-------------------------------+


struct fec_t
{
    fec_t(uint8_t* base, size_t size)
        : base(base)
        , max_size(size)
    {
    }

    void reset()
    {
        fec_type = nullptr;
        n = nullptr;
        data_blocks = nullptr;
        redu_blocks = nullptr;
        fec_d_sz = 0;
        fec_r_sz = 0;
        header_sz = 0;
        data_sz = 0;
    }

    void init(uint8_t fec, uint8_t n_)
    {
        if (max_size < 2)
        {
            return;
        }

        uint8_t *cursor = base;
        fec_type = cursor++;
        n = cursor++;
        *fec_type = fec;
        *n = n_;
        rescan();
    }

    void rescan()
    {
        reset();

        if (max_size < 2)
        {
            return;
        }

        uint8_t *cursor = base;
        fec_type = cursor++;
        n = cursor++;

        auto ft = (fec_type_e)(*fec_type);
        if      ( E_FEC_TYPE_NONE == ft)
        {
            fec_d_sz = 0;
            fec_r_sz = 0;
            data_blocks = n;
            n = nullptr;
            header_sz = 1;
            data_sz = max_size - header_sz;
            return;
        }
        else if (E_FEC_TYPE_RS_255_247 == ft)
        {
            fec_d_sz = 247;
            fec_r_sz = 255-247;
        }
        else if (E_FEC_TYPE_RS_255_239 == ft)
        {
            fec_d_sz = 239;
            fec_r_sz = 255-239;
        }
        else if (E_FEC_TYPE_RS_255_223 == ft)
        {
            fec_d_sz = 223;
            fec_r_sz = 255-223;
        }
        else if (E_FEC_TYPE_RS_255_191 == ft)
        {
            fec_d_sz = 191;
            fec_r_sz = 255-191;
        }
        else if (E_FEC_TYPE_RS_255_127  == ft)
        {
            fec_d_sz = 127;
            fec_r_sz = 255-127;
        }

        header_sz = 2;
        data_sz = *n*fec_d_sz;

        redu_blocks = cursor;
        if (*n*fec_d_sz + *n*fec_r_sz + 2 > max_size)
        {
            reset();
            return;
        }

        cursor += *n * fec_r_sz;
        data_blocks = cursor;
    }

    uint8_t* base = nullptr;
    llc_sz_t max_size = 0;
    size_t fec_d_sz = 0;
    size_t fec_r_sz = 0;
    size_t header_sz = 0;
    size_t data_sz = 0;

    uint8_t* fec_type = nullptr;
    uint8_t* n = nullptr;
    uint8_t* data_blocks = nullptr;
    uint8_t* redu_blocks = nullptr;
};

constexpr llc_sn_t llc_sn_mask = 0b11111111;
constexpr size_t llc_sn_size = llc_sn_mask+1;

//        +-------------------------------+
//     00 | LLC SN                        |
//        +---------------+---+-----------+
//     01 | LCID          | A | SIZEH     |
//        +---------------+---+-----------+
//     02 | SIZEL                         |
//        +-------------------------------+
//     03 | CRC                           | CRCSZ
//        +-------------------------------+
// HDR_SZ | Payload                       |
//        +-------------------------------+

struct llc_t
{
    llc_t(uint8_t* base, size_t size)
        : base(base)
        , max_size(size)
    {}

    static constexpr uint8_t mask_SN    = 0b11111111;
    static constexpr uint8_t mask_LCID  = 0b11110000;
    static constexpr uint8_t mask_A     = 0b00001000;
    static constexpr uint8_t mask_SIZEH = 0b00000111;
    static constexpr uint8_t mask_SIZEL = 0b11111111;

    static constexpr uint8_t shift_SN    = 0;
    static constexpr uint8_t shift_LCID  = 4;
    static constexpr uint8_t shift_A     = 3;
    static constexpr uint8_t shift_SIZEH = 0;
    static constexpr uint8_t shift_SIZEL = 0;

    uint8_t get(int index, uint8_t mask, uint8_t shift)
    {
        return (base[index] & mask) >> shift;
    }

    void set(int index, uint8_t mask, uint8_t shift, uint8_t val)
    {
        uint8_t mv = base[index] & ~mask;
        mv |= ((val<<shift) & mask);
        base[index] = mv;
    }

    void set_SN(llc_sn_t sn)
    {
        set(0, mask_SN, shift_SN, sn);
    }

    llc_sn_t get_SN()
    {
        return get(0, mask_SN, shift_SN);
    }

    void set_LCID(lcid_t lcid)
    {
        set(1, mask_LCID, shift_LCID, lcid);
    }

    lcid_t get_LCID()
    {
        return get(1, mask_LCID, shift_LCID);
    }

    void set_A(bool val)
    {
        set(1, mask_A, shift_A, val);
    }

    bool get_A()
    {
        return get(1, mask_A, shift_A);
    }

    void set_SIZE(llc_sz_t size)
    {
        set(1, mask_SIZEH, shift_SIZEH, size >> 8);
        set(2, mask_SIZEL, shift_SIZEL, size);
    }

    llc_sz_t get_SIZE()
    {
        return (get(1, mask_SIZEH, shift_SIZEH) << 8) |
               (get(2, mask_SIZEL, shift_SIZEL));
    }

    llc_sz_t get_header_size()
    {
        return 3 + crc_size;
    }

    uint8_t* payload()
    {
        return base+get_header_size();
    }

    llc_sz_t get_max_payload_size()
    {
        return max_size-get_header_size();
    }

    llc_sz_t get_payload_size()
    {
        return get_SIZE()-get_header_size();
    }

    void set_payload_size(llc_sz_t size)
    {
        set_SIZE(size+get_header_size());
    }

    uint8_t* CRC()
    {
        return max_size ? base+3 : nullptr;
    }

    uint8_t* base = nullptr;
    llc_sz_t max_size = 0;
    size_t crc_size = 0;
};

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

//    +-------------------------------+
//    | IV (OPTIONAL)                 | IVSZ
//    +-------------------------------+
//    | HMAC (OPTIONAL)               | HMSZ
//    +-------------------------------|
//    | PAYLOAD                       | EOP
//    +-------------------------------+

struct pdcp_t
{
    pdcp_t(uint8_t* base, size_t size)
        : base(base)
        , pdu_size(size)
    {
    }

    void rescan()
    {
        uint8_t *ptr = base;

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

    size_t get_header_size()
    {
        return (hmac_size + iv_size);
    }

    size_t iv_size = 0;
    size_t hmac_size = 0;

    uint8_t* iv = nullptr;
    uint8_t* hmac = nullptr;

    uint8_t* payload = nullptr;

    uint8_t* base = nullptr;
    llc_sz_t pdu_size = 0;
};

//     +---+---------------------------+
//  00 | L | SIZE                      | 02
//     +---+---------------------------|
//     | PACKET SN (OPTIONAL)          | 02
//     +-------------------------------+
//     | OFFSET (OPTIONAL)             | 02
//     +-------------------------------+
//     | PAYLOAD                       | EOS
//     +-------------------------------+

struct pdcp_segment_t
{
    pdcp_segment_t(uint8_t* base, size_t size)
        : base(base)
        , max_size(size)
    {
    }

    void rescan()
    {
        uint8_t *ptr = base;

        size = (winject::BEU16UA*) ptr;
        ptr += sizeof(*size);

        if (has_sn)
        {
            sn = (winject::BEU16UA*)ptr;
            ptr += sizeof(*sn);
        }

        if (has_offset)
        {
            offset = (winject::BEU16UA*) ptr;
            ptr += sizeof(*offset);
        }

        payload = ptr;
    }

    void set_SN(std::optional<pdcp_sn_t> value)
    {
        if (!value || sn==nullptr)
        {
            return;
        }

        *sn = *value;
    }

    std::optional<pdcp_sn_t> get_SN()
    {
        if (!has_sn || sn==nullptr)
        {
            return {};
        }
        return *sn;
    }

    void set_OFFSET(std::optional<pdcp_segment_offset_t> value)
    {
        if (!value || offset==nullptr)
        {
            return;
        }

        *offset = *value;
    }

    std::optional<pdcp_segment_offset_t> get_OFFSET()
    {
        if (!has_offset || offset==nullptr)
        {
            return {};
        }
        return *offset;
    }

    pdcp_segment_offset_t get_SIZE()
    {
        return *size & 0x7FFF;
    }

    void set_SIZE(pdcp_segment_offset_t size_)
    {
        *size = (*size&0x8000) | size_;
    }

    void set_LAST(bool is_last)
    {
        *size = (uint16_t(is_last) << 15) | get_SIZE();
    }

    bool is_LAST()
    {
        return *size & 0x8000;
    }

    size_t get_header_size()
    {
        return size_t(has_sn)*sizeof(*sn) +
            sizeof(*size) +
            size_t(has_offset)*sizeof(*offset);
    }

    pdcp_segment_offset_t get_payload_size()
    {
        return get_SIZE()-get_header_size();
    }

    void set_payload_size(pdcp_segment_offset_t payload_size)
    {
        set_SIZE(get_header_size() + payload_size);
    }

    uint8_t *base = nullptr;
    size_t max_size = 0;
    winject::BEU16UA *size = nullptr;
    bool has_sn = false;
    winject::BEU16UA *sn = nullptr;
    bool has_offset = false;
    winject::BEU16UA *offset = nullptr;
    uint8_t *payload = nullptr;
};

#endif // __WINJECTUM_FRAME_DEFS_HPP__
