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

#ifndef __WINJECT_802_11_FILTERS_hpp__
#define __WINJECT_802_11_FILTERS_hpp__

#include <cstdint>
#include <stddef.h>
#include <linux/filter.h> // BPF

namespace winject
{
namespace ieee_802_11
{
namespace filters
{

class data_addr3
{
public:
    data_addr3() = delete;

    data_addr3(uint64_t addr3)
    {
        set_addr3(addr3);
    }
    void set_addr3(uint64_t addr3)
    {
        constexpr int base = 10;
        filter_code[base+10].k = (addr3>>8*0) & 0xFF;
        filter_code[base+8].k  = (addr3>>8*1) & 0xFF;
        filter_code[base+6].k  = (addr3>>8*2) & 0xFF;
        filter_code[base+4].k  = (addr3>>8*3) & 0xFF;
        filter_code[base+2].k  = (addr3>>8*4) & 0xFF;
        filter_code[base+0].k  = (addr3>>8*5) & 0xFF;
    }

    sock_filter* data()
    {
        return filter_code;
    }

    unsigned short size() const
    {
        return sizeof(filter_code)/sizeof(sock_filter);
    }

private:
    sock_filter filter_code[23] =
    {
        { 0x30, 0, 0, 0x00000003 },     // [00]        ldb  [3]              ;  rt_len = radiotap.length_high;
        { 0x64, 0, 0, 0x00000008 },     // [01]        lsh  #8               ;  rt_len <<= 8;
        { 0x7, 0, 0, 0x00000000 },      // [02]        tax                   ;  
        { 0x30, 0, 0, 0x00000002 },     // [03]        ldb  [2]              ;
        { 0x4c, 0, 0, 0x00000000 },     // [04]        or   x                ;  rt_len |= radiotap.length_low;
        { 0x7, 0, 0, 0x00000000 },      // [05]        tax                   ;  
        { 0x50, 0, 0, 0x00000000 },     // [06]        ldb  [x + 0]          ;  
        { 0x54, 0, 0, 0x0000000f },     // [07]        and  #0x0F            ;  // TYPE=0b10 && PROTOCOL=0b00
        { 0x15, 0, 13, 0x00000008 },    // [08]        jne  #0x08, die       ;  if (0b1000 != fr80211.fc.proto_type) goto die; 
        { 0x50, 0, 0, 0x00000010 },     // [09]        ldb  [x + 16]         ;  // FC(2) + DURATION(2) + ADDR1(6) + ADDR2(6)
        { 0x15, 0, 11, 0x000000ef },    // [10]        jne  #0xEF, die       ;  if (imm != fr80211.addr3[0]) goto die;
        { 0x50, 0, 0, 0x00000011 },     // [11]        ldb  [x + 17]         ;  
        { 0x15, 0, 9, 0x000000be },     // [12]        jne  #0xBE, die       ;  if (imm != fr80211.addr3[1]) goto die;
        { 0x50, 0, 0, 0x00000012 },     // [13]        ldb  [x + 18]         ;  
        { 0x15, 0, 7, 0x000000ad },     // [14]        jne  #0xAD, die       ;  if (imm != fr80211.addr3[2]) goto die;
        { 0x50, 0, 0, 0x00000013 },     // [15]        ldb  [x + 19]         ;  
        { 0x15, 0, 5, 0x000000de },     // [16]        jne  #0xDE, die       ;  if (imm != fr80211.addr3[3]) goto die;
        { 0x50, 0, 0, 0x00000014 },     // [17]        ldb  [x + 20]         ;  
        { 0x15, 0, 3, 0x000000af },     // [18]        jne  #0xAF, die       ;  if (imm != fr80211.addr3[4]) goto die;
        { 0x50, 0, 0, 0x00000015 },     // [19]        ldb  [x + 21]         ;  
        { 0x15, 0, 1, 0x000000FA },     // [20]        jne  #0xFA, die       ;  if (imm != fr80211.addr3[5]) goto die;
        { 0x6, 0, 0, 0xffffffff },      // [21]        ret #-1               ;  return -1;
        { 0x6, 0, 0, 0x00000000 },      // [22]  die:  ret #0                ;  return 0;
    };
};

class winject_rts_src
{
public:
    winject_rts_src() = delete;

    winject_rts_src(uint64_t src)
    {
        set_src(src);
    }
    void set_src(uint64_t src)
    {
        // filter_code[10].k = 0xDEADBEEF;
        // filter_code[12].k = 0xCAFE; 
        uint16_t src_ = (src>>8)&0xFFFF;
        filter_code[14].k = src_;
        filter_code[16].k = src&0xFF;
    }

    sock_filter* data()
    {
        return filter_code;
    }

    unsigned short size() const
    {
        return sizeof(filter_code)/sizeof(sock_filter);
    }

private:
    sock_filter filter_code[19] =
    {
        { 0x30, 0, 0, 0x00000003 },           // [00]       ldb  [3]              ;  rt_len = radiotap.length_high;
        { 0x64, 0, 0, 0x00000008 },           // [01]       lsh  #8               ;  rt_len <<= 8;
        { 0x7, 0, 0, 0x00000000 },            // [02]       tax                   ;  
        { 0x30, 0, 0, 0x00000002 },           // [03]       ldb  [2]              ;
        { 0x4c, 0, 0, 0x00000000 },           // [04]       or   x                ;  rt_len |= radiotap.length_low;
        { 0x7, 0, 0, 0x00000000 },            // [05]       tax                   ;  
        { 0x50, 0, 0, 0x00000000 },           // [06]       ldb  [x + 0]          ;  
        { 0x54, 0, 0, 0x000000ff },           // [07]       and  #0xFF            ;  // PROTOCOL=0b00 && TYPE=0b01 && SUBTYPE=0b1011
        { 0x15, 0, 9, 0x000000b4 },           // [08]       jne  #0xb4, die       ;  if (1 != fr80211.fc.type && 0xb!= fr80211.fc.subtype) goto die;
        { 0x40, 0, 0, 0x0000000a },           // [09]       ld   [x + 10]         ;  // FC(2) + DURATION(2) + ADDR1(6)
        { 0x15, 0, 7, 0xdeadbeef },           // [10]       jne  #0xDEADBEEF, die ;      
        { 0x48, 0, 0, 0x00000014 },           // [11]       ldh  [x + 20]         ;  
        { 0x15, 0, 5, 0x0000cafe },           // [12]       jne  #0xCAFE, die     ;  if (0xDEADBEEFCAFE != fr80211.addr2) goto die;
        { 0x48, 0, 0, 0x00000004 },           // [13]       ldh  [x + 4]          ;
        { 0x15, 0, 3, 0x0000ffff },           // [14]       jne  #0xFFFF, die     ;
        { 0x50, 0, 0, 0x00000006 },           // [15]       ldb  [x + 6]          ;
        { 0x15, 0, 1, 0x000000ff },           // [16]       jne  #0xFF, die       ;  if (0xFFFFFF != fr80211.addr1 >> 24)
        { 0x6, 0, 0, 0xffffffff },            // [17]       ret #-1               ;  return -1;     
        { 0x6, 0, 0, 0x00000000 },            // [18] die:  ret #0                ;  return 0;
    };
};

class winject_src
{
public:
    winject_src() = delete;

    winject_src(uint32_t src)
    {
        set_src(src);
    }

    void set_src(uint32_t src)
    {
        // filter_code[10].k = 0xDEADBEEF;
        // filter_code[12].k = 0xCAFE; 
        uint16_t src_ = (src>>8)&0xFFFF;
        filter_code[18].k = src_;
        filter_code[20].k = src&0xFF;
    }

    sock_filter* data()
    {
        return filter_code;
    }

    unsigned short size() const
    {
        return sizeof(filter_code)/sizeof(sock_filter);
    }

private:
    static constexpr int LONGEST=13;
    sock_filter filter_code[23] =
    {
        { 0x30, 0, 0, 0x00000003 },           // [00]      ldb  [3]              ;  rt_len = radiotap.length_high;
        { 0x64, 0, 0, 0x00000008 },           // [01]      lsh  #8               ;  rt_len <<= 8;
        { 0x7, 0, 0, 0x00000000 },            // [02]      tax                   ;  
        { 0x30, 0, 0, 0x00000002 },           // [03]      ldb  [2]              ;
        { 0x4c, 0, 0, 0x00000000 },           // [04]      or   x                ;  rt_len |= radiotap.length_low;
        { 0x7, 0, 0, 0x00000000 },            // [05]      tax                   ;  
        { 0x50, 0, 0, 0x00000000 },           // [06]      ldb  [x + 0]          ;  
        { 0x54, 0, 0, 0x0000000f },           // [07]      and  #0x0F            ;  // TYPE=0b10 && PROTOCOL=0b00
        { 0x15, 0, LONGEST, 0x00000008 },     // [08]      jne  #0x08, die       ;  if (0b1000 != fr80211.fc.proto_type) goto die;
        { 0x40, 0, 0, 0x00000010 },           // [09]      ld   [x + 16]         ;  // FC(2) + DURATION(2) + ADDR1(6) + ADDR2(6)
        { 0x15, 0, LONGEST-2, 0xdeadbeef },   // [10]      jne  #0xDEADBEEF, die ;      
        { 0x48, 0, 0, 0x00000014 },           // [11]      ldh  [x + 20]         ;  
        { 0x15, 0, LONGEST-4, 0x0000cafe },   // [12]      jne  #0xCAFE, die     ;  if (0xDEADBEEFCAFE != fr80211.addr3) goto die;
        { 0x40, 0, 0, 0x00000004 },           // [13]      ld   [x + 4]          ;
        { 0x15, 0, LONGEST-6, 0xffffffff },   // [14]      jne  #0xFFFFFFFF, die ;
        { 0x48, 0, 0, 0x00000004 },           // [15]      ldh  [x + 4]          ;
        { 0x15, 0, LONGEST-8, 0x0000ffff },   // [16]      jne  #0xFFFF, die     ;  if (0xFFFFFFFFFF != fr80211.addr1) goto die
        { 0x48, 0, 0, 0x0000000a },           // [17]      ldh  [x + 10]         ;
        { 0x15, 0, LONGEST-10, 0x0000ffaa },  // [18]      jne  #0xFFFF, die     ;
        { 0x50, 0, 0, 0x0000000c },           // [19]      ldb  [x + 12]         ;
        { 0x15, 0, LONGEST-12, 0x00000000 },  // [20]      jne  #0xFF, die       ;  if (0xFFFFFF != fr80211.addr2 >> 24)
        { 0x6, 0, 0, 0xffffffff },            // [++]      ret #-1               ;  return -1;     
        { 0x6, 0, 0, 0x00000000 },            // [22] die: ret #0                ;  return 0;
    };
};

template <typename T>
inline int attach(int sd, T& filter)
{
    sock_fprog bpf =
        {
            .len    = filter.size(),
            .filter = filter.data(),
        };

    return setsockopt(sd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf));
}

} // namespace filters
} // namespace ieee_802_11
} // namespace winject

#endif // __WINJECT_802_11_FILTERS_hpp__