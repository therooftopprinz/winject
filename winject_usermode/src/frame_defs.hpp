#ifndef __WINJECTUM_FRAME_DEFS_HPP__
#define __WINJECTUM_FRAME_DEFS_HPP__

#include <cstdint>
#include <cstddef>

using llc_sn_t = uint8_t;
using lcid_t = uint8_t;
using llc_sz_t = uint16_t;

//    +---+---+-----------------------+
// 00 | D | R | LLC SN                |
//    +---+---+-------+---+-----------+
// 01 | LCID          | C | SIZEH     |
//    +---------------+---+-----------+
// 02 | SIZEL                         |
//    +-------------------------------+
// 03 | Payload                       |
//    +-------------------------------+
struct llc_t
{
    llc_t(uint8_t* base, size_t size)
        : base(base)
        , max_size(size)
    {}

    static constexpr uint8_t mask_D     = 0b10000000;
    static constexpr uint8_t mask_R     = 0b01000000;
    static constexpr uint8_t mask_SN    = 0b00111111;
    static constexpr uint8_t mask_LCID  = 0b11110000;
    static constexpr uint8_t mask_A     = 0b00001000;
    static constexpr uint8_t mask_SIZEH = 0b00000111;
    static constexpr uint8_t mask_SIZEL = 0b11111111;
    static constexpr uint8_t shift_D     = 7;
    static constexpr uint8_t shift_R     = 6;
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

    void set_D(bool val)
    {
        set(0, mask_D, shift_D, val);
    }

    bool get_D()
    {
        return get(0, mask_D, shift_D);
    }

    void set_R(bool val)
    {
        set(0, mask_R, shift_R, val);
    }

    bool get_R()
    {
        return get(0, mask_R, shift_R);
    }

    void set_SN(sn_t sn)
    {
        set(0, mask_SN, shift_SN, sn);
    }

    sn_t get_SN()
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

    static llc_sz_t get_header_size()
    {
        return 3;
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


    uint8_t* base = nullptr;
    llc_sz_t max_size = 0;
};

// LLC-CTL-ACK Payload:
//    +---+---+-----------------------+
// 00 | # | # | LLC SN                |
//    +---+---+-----------------------+
// 01 | # | # | COUNT                 |
//    +---+---+-----------------------+
struct llc_payload_ack_t
{
    sn_t sn;
    sn_t count;
} __attribute((packed));

//     +-------------------------------+
//  00 | PDCP SN                       |
//     +-------------------------------+
//  01 | OFFSET (OPTIONAL)             |
//     +-------------------------------|
//  04 | PAYLOAD                       |
//     +-------------------------------+
//  SZ | MAC                           | EOP
//     +-------------------------------+

struct pdcp_t
{
    sn_t 
    uint8_t* base = nullptr;
    llc_sz_t max_size = 0;
};

struct pdcp_uf_t
{
    uint8_t* base = nullptr;
    llc_sz_t max_size = 0;
};

#endif // __WINJECTUM_FRAME_DEFS_HPP__