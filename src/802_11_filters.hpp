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

} // namespace filters
} // namespace ieee_802_11
} // namespace winject

#endif // __WINJECT_802_11_FILTERS_hpp__