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
#include <type_traits>
#include "safeint.hpp"
#include "info_defs.hpp"

using llc_sn_t = uint8_t;
using lcid_t = uint8_t;
using llc_sz_t = uint16_t;
using pdcp_sn_t = uint16_t;
using pdcp_segment_offset_t = uint16_t;

struct safe_checker
{
public:
    safe_checker(uint8_t* start, uint8_t* end)
        : start(start)
        , current(start)
        , end(end)
        , validity(true)
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
    bool validity = false;
};

//     +-------------------------------+
//     | FEC_TYPE                      | 01
//     +-------------------------------+
//     | N (FEC_TYPE != 0)             | 01 * (FEC_TYPE!=0)
//     +-------------------------------+
//     | REDUNDANCY                    | FEC_R_SZ * N
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

    bool is_valid()
    {
        return fec_type != nullptr;
    }

    void init(uint8_t fec, uint8_t n_)
    {
        safe_checker checker(base, base + max_size);
        fec_type = checker.get<uint8_t>();
        rescan();
        if (n)
        {
            *n = n_;
        }
    }

    void rescan()
    {
        safe_checker checker(base, base + max_size);
        fec_type = checker.get<uint8_t>();
        if (!checker) return reset();

        auto ft = (fec_type_e)(*fec_type);
        if (ft != E_FEC_TYPE_NONE)
        {
            n = checker.get<uint8_t>();
            if (!checker) return reset();
        }

        if (E_FEC_TYPE_RS_255_247 == ft)
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

        redu_blocks = checker.get_n(*n*fec_r_sz);
        if (!checker) return reset();
        data_blocks = checker.get_n(*n*fec_d_sz);
        if (!checker) return reset();
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

    bool is_valid()
    {
        return get_SIZE() > max_size;
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
//    | PAYLOAD                       |
//    +-------------------------------+


// @note: Deprecated, pdcp is now composed of pdcp_segment_t only, encryption moved to upper layer.
struct pdcp_t
{
    pdcp_t(uint8_t* base, size_t size)
        : base(base)
        , max_size(size)
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

    bool is_header_valid()
    {
        return max_size > get_header_size();
    }

    size_t iv_size = 0;
    size_t hmac_size = 0;

    uint8_t* iv = nullptr;
    uint8_t* hmac = nullptr;

    uint8_t* payload = nullptr;

    uint8_t* base = nullptr;
    size_t   max_size = 0;
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


template <bool IsConst>
struct basic_pdcp_segment_t
{
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;
    using u16_ptr  = std::conditional_t<IsConst, const winject::BEU16UA*, winject::BEU16UA*>;

    basic_pdcp_segment_t(byte_ptr base, size_t size)
        : base(base)
        , max_size(size)
    {}

    void reset()
    {
        size = nullptr;
        sn = nullptr;
        offset = nullptr;
        payload = nullptr;
    }

    void rescan()
    {
        auto start = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(base));
        safe_checker checker(start, start + max_size);

        size = checker.get<winject::BEU16UA>();
        if (!checker) return reset();

        if (has_sn)
        {
            sn = checker.get<winject::BEU16UA>();
            if (!checker) return reset();
        }

        if (has_offset)
        {
            offset = checker.get<winject::BEU16UA>();
            if (!checker) return reset();
        }

        payload = checker.get_n(checker.remaining());
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SN(std::optional<pdcp_sn_t> value)
    {
        if (!value || sn==nullptr)
        {
            return;
        }

        *sn = *value;
    }

    std::optional<pdcp_sn_t> get_SN() const
    {
        if (!has_sn || sn == nullptr)
        {
            return {};
        }
        return *sn;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_OFFSET(std::optional<pdcp_segment_offset_t> value)
    {
        if (!value || offset == nullptr)
        {
            return;
        }
        *offset = *value;
    }

    std::optional<pdcp_segment_offset_t> get_OFFSET() const
    {
        if (!has_offset || offset == nullptr)
        {
            return {};
        }
        return *offset;
    }

    pdcp_segment_offset_t get_SIZE() const
    {
        return size ? (static_cast<pdcp_segment_offset_t>(*size) & 0x7FFF) : 0;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SIZE(pdcp_segment_offset_t size_)
    {
        if (!size)
        {
            return;
        }
        *size = (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) | size_;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_LAST(bool is_last)
    {
        if (!size)
        {
            return;
        }
        *size = (uint16_t(is_last) << 15) | get_SIZE();
    }

    bool is_LAST() const
    {
        return size ? (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) != 0 : false;
    }

    size_t get_header_size() const
    {
        return size_t(has_sn)*sizeof(*sn) +
            sizeof(*size) +
            size_t(has_offset)*sizeof(*offset);
    }

    pdcp_segment_offset_t get_payload_size() const
    {
        return get_SIZE()-get_header_size();
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_payload_size(pdcp_segment_offset_t payload_size)
    {
        set_SIZE(get_header_size() + payload_size);
    }

    bool is_header_valid() const
    {
        return max_size > get_header_size();
    }

    bool is_valid() const
    {
        return is_header_valid() && max_size > get_payload_size();;
    }

    byte_ptr base = nullptr;
    size_t max_size = 0;
    u16_ptr size = nullptr;
    bool has_sn = false;
    u16_ptr sn = nullptr;
    bool has_offset = false;
    u16_ptr offset = nullptr;
    byte_ptr payload = nullptr;
};

using pdcp_segment_t       = basic_pdcp_segment_t<false>;
using pdcp_segment_const_t = basic_pdcp_segment_t<true>;

#endif // __WINJECTUM_FRAME_DEFS_HPP__
