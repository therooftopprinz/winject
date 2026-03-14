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

#ifndef __WINJECT_FRAME_DEFS_HPP__
#define __WINJECT_FRAME_DEFS_HPP__

#include <cstdint>
#include <cstddef>
#include <optional>
#include <type_traits>
#include <cstring>
#include "safeint.hpp"
#include "info_defs.hpp"

using llc_sn_t = uint8_t;
using lcid_t = uint8_t;
using llc_sz_t = uint16_t;
using pdcp_sn_t = uint16_t;
using pdcp_segment_offset_t = uint16_t;

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

//     +-------------------------------+
//     | FEC_TYPE                      | 01
//     +-------------------------------+
//     | N (FEC_TYPE != 0)             | 01 * (FEC_TYPE!=0)
//     +-------------------------------+
//     | REDUNDANCY                    | FEC_R_SZ * N
//     +-------------------------------+
//     | DATA                          | FEC_D_SZ * N
//     +-------------------------------+


template <bool IsConst>
class basic_fec_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;

    basic_fec_t(byte_ptr base, size_t size)
        : base(base)
        , max_size(size)
    {}

    void reset()
    {
        fec_type    = nullptr;
        n           = nullptr;
        data_blocks = nullptr;
        redu_blocks = nullptr;
        fec_d_sz    = 0;
        fec_r_sz    = 0;
        is_valid_   = false;
    }

    bool is_valid() const
    {
        return is_valid_;
    }

    byte_ptr get_base() const
    {
        return base;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void init(uint8_t fec, uint8_t n_)
    {
        is_valid_ = true;
        safe_checker checker(base, base + max_size, is_valid_);
        fec_type = checker.get<uint8_t>();
        if (!is_valid()) return;
        *fec_type = fec;

        rescan();

        if (n)
        {
            *n = n_;
        }
    }

    void rescan()
    {
        is_valid_ = true;
        auto start = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(base));
        safe_checker checker(start, start + max_size, is_valid_);
        fec_type = checker.get<uint8_t>();
        if (!is_valid()) return reset();

        auto ft = (fec_type_e)(*fec_type);
        if (ft != E_FEC_TYPE_NONE)
        {
            n = checker.get<uint8_t>();
            if (!is_valid()) return;
        }

        if (E_FEC_TYPE_RS_255_247 == ft)
        {
            fec_d_sz = 247;
            fec_r_sz = 255 - 247;
        }
        else if (E_FEC_TYPE_RS_255_239 == ft)
        {
            fec_d_sz = 239;
            fec_r_sz = 255 - 239;
        }
        else if (E_FEC_TYPE_RS_255_223 == ft)
        {
            fec_d_sz = 223;
            fec_r_sz = 255 - 223;
        }
        else if (E_FEC_TYPE_RS_255_191 == ft)
        {
            fec_d_sz = 191;
            fec_r_sz = 255 - 191;
        }
        else if (E_FEC_TYPE_RS_255_127 == ft)
        {
            fec_d_sz = 127;
            fec_r_sz = 255 - 127;
        }

        redu_blocks = checker.get_n(*n * fec_r_sz);
        if (!is_valid()) return;
        data_blocks = checker.get_n(*n * fec_d_sz);
        if (!is_valid()) return;
    }

    uint8_t get_FEC_TYPE() const
    {
        return *fec_type;
    }

    uint8_t get_N() const
    {
        return *n;
    }

    byte_ptr get_data_blocks() const
    {
        return data_blocks;
    }

    byte_ptr get_redu_blocks() const
    {
        return redu_blocks;
    }

    size_t get_fec_d_sz() const
    {
        return fec_d_sz;
    }

    size_t get_fec_r_sz() const
    {
        return fec_r_sz;
    }

    size_t get_data_sz() const
    {
        return get_N() * get_fec_d_sz();
    }

    size_t get_redu_sz() const
    {
        return get_N() * get_fec_r_sz();
    }

private:
    byte_ptr base         = nullptr;
    llc_sz_t max_size     = 0;
    size_t   fec_d_sz     = 0;
    size_t   fec_r_sz     = 0;

    byte_ptr fec_type     = nullptr;
    byte_ptr n            = nullptr;
    byte_ptr data_blocks  = nullptr;
    byte_ptr redu_blocks  = nullptr;
    bool     is_valid_    = false;
};

using fec_t       = basic_fec_t<false>;
using fec_const_t = basic_fec_t<true>;

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

template <bool IsConst>
class basic_llc_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;


    basic_llc_t() = delete;

    basic_llc_t(byte_ptr base, size_t size, size_t crc_size = 0)
        : base(base)
        , max_size(size)
        , crc_size(crc_size)
    {}

    basic_llc_t(const basic_llc_t& other)
        : base(other.base)
        , max_size(other.max_size)
        , crc_size(other.crc_size)
    {}

    basic_llc_t& operator=(const basic_llc_t& other)
    {
        base = other.base;
        max_size = other.max_size;
        crc_size = other.crc_size;
        return *this;
    }

    void rebase(byte_ptr new_base, size_t new_max_size)
    {
        base = new_base;
        max_size = new_max_size;
    }

    byte_ptr get_base() const
    {
        return base;
    }

    size_t get_max_size() const
    {
        return max_size;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SN(llc_sn_t sn)
    {
        set(0, mask_SN, shift_SN, sn);
    }

    llc_sn_t get_SN() const
    {
        return get(0, mask_SN, shift_SN);
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_LCID(lcid_t lcid)
    {
        set(1, mask_LCID, shift_LCID, lcid);
    }

    lcid_t get_LCID() const
    {
        return get(1, mask_LCID, shift_LCID);
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_A(bool val)
    {
        set(1, mask_A, shift_A, val);
    }

    bool get_A() const
    {
        return get(1, mask_A, shift_A);
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SIZE(llc_sz_t size)
    {
        set(1, mask_SIZEH, shift_SIZEH, size >> 8);
        set(2, mask_SIZEL, shift_SIZEL, size);
    }

    llc_sz_t get_SIZE() const
    {
        return (get(1, mask_SIZEH, shift_SIZEH) << 8) |
               (get(2, mask_SIZEL, shift_SIZEL));
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_CRC(byte_ptr crc)
    {
        std::memcpy(base + 3, crc, crc_size);
    }

    byte_ptr get_CRC() const
    {
        return base + 3;
    }

    llc_sz_t get_header_size() const
    {
        return 3 + crc_size;
    }

    byte_ptr get_payload() const
    {
        return base + get_header_size();
    }

    llc_sz_t get_max_payload_size() const
    {
        return max_size - get_header_size();
    }

    llc_sz_t get_payload_size() const
    {
        return get_SIZE() - get_header_size();
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_payload_size(llc_sz_t size)
    {
        set_SIZE(size + get_header_size());
    }

    bool is_valid() const
    {
        return get_SIZE() > max_size;
    }

    bool is_header_valid() const
    {
        return max_size >= get_header_size();
    }

private:
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

    uint8_t get(int index, uint8_t mask, uint8_t shift) const
    {
        return (base[index] & mask) >> shift;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set(int index, uint8_t mask, uint8_t shift, uint8_t val)
    {
        uint8_t mv = base[index] & ~mask;
        mv |= ((val << shift) & mask);
        base[index] = mv;
    }

    byte_ptr     base     = nullptr;
    llc_sz_t     max_size = 0;
    const size_t crc_size = 0;
};

using llc_t       = basic_llc_t<false>;
using llc_const_t = basic_llc_t<true>;

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

// @brief PDCP segment frame definition
//        Handles set and get for PDCP Segment
// @note UB when accessing fields without proper configuration.
//       You must check is_valid() after rescan() or set_SIZE() to ensure the segment is valid.
template <bool IsConst>
class basic_pdcp_segment_t
{
public:
    using byte_ptr = std::conditional_t<IsConst, const uint8_t*, uint8_t*>;
    using u16_ptr  = std::conditional_t<IsConst, const winject::BEU16UA*, winject::BEU16UA*>;

    basic_pdcp_segment_t() = delete;
    basic_pdcp_segment_t(byte_ptr base, size_t size, bool has_sn = false, bool has_offset = false)
        : base(base)
        , max_size(size)
        , has_sn(has_sn)
        , has_offset(has_offset)
    {}

    void rebase(byte_ptr new_base, size_t new_size)
    {
        base = new_base;
        max_size = new_size;
        rescan();
    }

    byte_ptr get_base() const
    {
        return base;
    }

    size_t get_max_size() const
    {
        return max_size;
    }

    void reset()
    {
        size    = nullptr;
        sn      = nullptr;
        offset  = nullptr;
        payload = nullptr;
    }

    bool rescan()
    {
        is_valid_ = true;
        auto start = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(base));
        safe_checker checker(start, start + max_size, is_valid_);

        size = checker.get<winject::BEU16UA>();
        if (!is_valid()) return false;

        if (has_sn)
        {
            sn = checker.get<winject::BEU16UA>();
            if (!is_valid()) return false;
        }

        if (has_offset)
        {
            offset = checker.get<winject::BEU16UA>();
            if (!is_valid()) return false;
        }

        payload = checker.get_n(checker.remaining());
        return is_valid();
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SN(pdcp_sn_t value)
    {
        *sn = value;
    }

    pdcp_sn_t get_SN() const
    {
        return *sn;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_OFFSET(pdcp_segment_offset_t value)
    {
        *offset = value;
    }

    pdcp_segment_offset_t get_OFFSET() const
    {
        return *offset;
    }

    pdcp_segment_offset_t get_SIZE() const
    {
        return size ? (static_cast<pdcp_segment_offset_t>(*size) & 0x7FFF) : 0;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_SIZE(pdcp_segment_offset_t size_)
    {
        *size = (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) | size_;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    void set_LAST(bool is_last)
    {
        *size = (uint16_t(is_last) << 15) | get_SIZE();
    }

    bool is_LAST() const
    {
        return (static_cast<pdcp_segment_offset_t>(*size) & 0x8000) != 0;
    }

    size_t get_header_size() const
    {
        return sizeof(*size) +
               has_sn        * sizeof(*sn) +
               has_offset    * sizeof(*offset);
    }

    pdcp_segment_offset_t get_payload_size() const
    {
        return get_SIZE()-get_header_size();
    }

    byte_ptr get_payload() const
    {
        return payload;
    }

    template <bool B = IsConst, typename = std::enable_if_t<!B>>
    bool set_payload_size(pdcp_segment_offset_t payload_size)
    {
        auto new_size = get_header_size() + payload_size;
        set_SIZE(get_header_size() + payload_size);
        if (new_size > max_size)
        {
            is_valid_ = false;
        }
        return is_valid();
    }

    bool is_header_valid() const
    {
        return max_size >= get_header_size();
    }

    bool is_valid()
    {
        return is_valid_;
    }

private:
    byte_ptr base       = nullptr;
    size_t   max_size   = 0;
    u16_ptr  size       = nullptr;
    bool     has_sn     = false;
    u16_ptr  sn         = nullptr;
    bool     has_offset = false;
    u16_ptr  offset     = nullptr;
    byte_ptr payload    = nullptr;
    bool     is_valid_   = false;
};

using pdcp_segment_t       = basic_pdcp_segment_t<false>;
using pdcp_segment_const_t = basic_pdcp_segment_t<true>;

#endif // __WINJECT_FRAME_DEFS_HPP__
