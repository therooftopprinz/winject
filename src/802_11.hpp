#ifndef __WINJECT_802_11_hpp__
#define __WINJECT_802_11_hpp__

#include <cstdint>

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

    uint64_t get()
    {
        uint64_t rv = 0;

        rv |= uint64_t(address[0]) << (40);
        rv |= uint64_t(address[1]) << (32);
        rv |= uint64_t(address[2]) << (24);
        rv |= uint64_t(address[3]) << (16);
        rv |= uint64_t(address[4]) << (8);
        rv |= uint64_t(address[5]) << (0);

        return rv;
    }

    void set(uint64_t value)
    {
        address[0] = (value >> 40) & 0xFF;
        address[1] = (value >> 32) & 0xFF;
        address[2] = (value >> 24) & 0xFF;
        address[3] = (value >> 16) & 0xFF;
        address[4] = (value >> 8)  & 0xFF;
        address[5] = (value >> 0)  & 0xFF;
    }
} __attribute__((__packed__));

struct seq_ctl_t
{
    // @todo : not working in big endian
    uint16_t frag_sec;

    uint8_t get_fragment_num()
    {
        return le16toh(frag_sec) & 0xF;
    }

    void set_fragment_num(uint8_t value)
    {
        frag_sec = le16toh(frag_sec) & 0xFFF0;
        frag_sec = le16toh(frag_sec) | value;
    }

    uint16_t get_seq_num()
    {
        return le16toh(frag_sec) >> 4;
    }

    void set_seq_num(uint16_t value)
    {
        frag_sec = le16toh(frag_sec) & 0xF;
        frag_sec = le16toh(frag_sec) | (value << 4);
    }

} __attribute__((__packed__));

class frame_t
{
public:
    frame_t(uint8_t* buffer)
    {
        if (!buffer)
        {
            return;
        }

        frame_control = (frame_control_t*) buffer;
        duration = (uint16_t*)(buffer + sizeof(frame_control_t));

        rescan();
    }

    void reset()
    {
        address1 = nullptr;
        address2 = nullptr;
        address3 = nullptr;
        seq_ctl = nullptr;
        address4 = nullptr;
        frame_body = nullptr;
        fcs = nullptr;
    }

    void rescan()
    {
        reset();

        auto type = frame_control->protocol_type & frame_control_t::E_TYPE_MASK;
        auto mtype = type & frame_control_t::E_MTYPE_MASK;
        auto stype = type & frame_control_t::E_TYPE_MASK;
        if (mtype == frame_control_t::E_MTYPE_DATA)
        {
            address1 = (address_t*)((uint8_t*)duration + sizeof(uint16_t));
            address2 = (address_t*)((uint8_t*)address1 + sizeof(address_t));
            address3 = (address_t*)((uint8_t*)address2 + sizeof(address_t));
            seq_ctl  = (seq_ctl_t*)((uint8_t*)address3 + sizeof(address_t));

            if ((frame_control->flags & frame_control_t::E_FLAGS_TO_DS) &&
                (frame_control->flags & frame_control_t::E_FLAGS_FROM_DS))
            {
                address4 = (address_t*)((uint8_t*)seq_ctl + sizeof(seq_ctl_t));
                frame_body = (uint8_t*)address4 + sizeof(address_t);
            }
            else
            {
                address4 = nullptr;
                frame_body = (uint8_t*)seq_ctl + sizeof(seq_ctl_t);
            }

            fcs = (uint32_t*)frame_body;
        }
        else if (stype == frame_control_t::E_TYPE_RTS)
        {
            address1 = (address_t*)((uint8_t*)duration + sizeof(uint16_t));
            address2 = (address_t*)((uint8_t*)address1 + sizeof(address_t));
            frame_body = (uint8_t*)address2 + sizeof(address_t);
            fcs = (uint32_t*)frame_body;
        }
    }

    uint8_t* end()
    {
        return (uint8_t*)fcs + sizeof(fcs);
    }

    size_t size()
    {
        return end() - (uint8_t*)frame_control + (fcs ? 4 : 0);
    }

    void set_body_size(uint16_t size)
    {
        fcs = (uint32_t*)(frame_body+size);
    }

    frame_control_t* frame_control = nullptr;
    uint16_t* duration = nullptr;
    address_t* address1 = nullptr;
    address_t* address2 = nullptr;
    address_t* address3 = nullptr;
    seq_ctl_t* seq_ctl = nullptr;
    address_t* address4 = nullptr;
    uint8_t* frame_body = nullptr;
    uint32_t* fcs = nullptr;
};

inline std::string to_string(frame_t& frame80211)
{
    char buffer[1024];
    char *current = buffer;
    auto rem = [buffer, &current]() {
            return uintptr_t(buffer+sizeof(buffer)-current);
        };

    auto check = [&current](int result) mutable {
            if (result<0)
                return;
            current += result;
        };

    check(snprintf(current, rem(), "802.11:\n"));
    check(snprintf(current, rem(), "  frame_control:\n"));
    check(snprintf(current, rem(), "    protocol_type: %p\n", (void*)(uintptr_t) frame80211.frame_control->protocol_type));
    check(snprintf(current, rem(), "    flags: %p\n", (void*)(uintptr_t) frame80211.frame_control->flags));
    check(snprintf(current, rem(), "  duration: %p\n", (void*)(uintptr_t) *frame80211.duration));
    if (frame80211.address1)
        check(snprintf(current, rem(), "  address1: %p\n", (void*) frame80211.address1->get()));
    if (frame80211.address2)
        check(snprintf(current, rem(), "  address2: %p\n", (void*) frame80211.address2->get()));
    if (frame80211.address3)
        check(snprintf(current, rem(), "  address3: %p\n", (void*) frame80211.address3->get()));
    if (frame80211.address3)
    {
        check(snprintf(current, rem(), "  seq_ctl:\n"));
        check(snprintf(current, rem(), "    seq: %d\n", frame80211.seq_ctl->get_seq_num()));
        check(snprintf(current, rem(), "    frag: %d\n", frame80211.seq_ctl->get_fragment_num()));
    }
    return buffer;
}

} // namespace 802_11
} // namespace winject

#endif // __WINJECT_802_11_hpp__