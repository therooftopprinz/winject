#ifndef __WINJECT_RADIOTAP_HPP__
#define __WINJECT_RADIOTAP_HPP__

#include <cstdint>

namespace winject
{
namespace radiotap
{

struct header_t
{
    u_int8_t  version;
    u_int8_t  pad;
    u_int16_t length;
    u_int32_t presence;
} __attribute__((__packed__));

struct tsft_t              // size  8 align  8
{
    uint64_t mactime;
};

struct flags_t             // size  1 align  1
{
    enum flags_e : uint8_t
    {
        E_FLAGS_CFP = 0x01,
        E_FLAGS_SPRE = 0x02,
        E_FLAGS_WEP = 0x04,
        E_FLAGS_FRAG = 0x08,
        E_FLAGS_FCS = 0x10,
        E_FLAGS_DATAPAD = 0x20,
        E_FLAGS_BADFCS = 0x40,
        E_FLAGS_SGI = 0x80
    };
    uint8_t flags;
};

struct rate_t              // size  1 align  1
{
    uint8_t value;
};

struct channel_t           // size  4 align  2
{
    enum flags_e : uint16_t
    {
	    E_FLAGS_TURBO = 0x0010,
	    E_FLAGS_CCK = 0x0020,
	    E_FLAGS_OFDM = 0x0040,
	    E_FLAGS_2GHZ = 0x0080,
	    E_FLAGS_5GHZ = 0x0100,
	    E_FLAGS_PASSIVE = 0x0200,
	    E_FLAGS_DYN = 0x0400,
	    E_FLAGS_GFSK = 0x0800,
	    E_FLAGS_HALF = 0x4000,
	    E_FLAGS_QUARTER = 0x8000
    };
    uint16_t frequency;
    uint16_t flags;
};

struct fhss_t              // size  2 align  2
{
    // note : implement properly
    uint16_t data;

    void set_hop_set(uint8_t value)
    {
        hop_set() = value;
    }

    uint8_t get_hop_set()
    {
        return hop_set();
    }

    void set_hop_pattern(uint8_t value)
    {
        hop_pattern() = value;
    }

    uint8_t get_hop_pattern()
    {
        return hop_pattern();
    }

    uint8_t& hop_set()
    {
        return *((uint8_t*)&data);
    }
    uint8_t& hop_pattern()
    {
        return *((uint8_t*)&data+1);
    }
};

struct antenna_signal_t    // size  1 align  1
{
    int8_t dbm_value;
};

struct antenna_noise_t     // size  1 align  1
{
    int8_t dbm_value;
};

struct lock_quality_t      // size  2 align  2
{
    uint16_t value;
};

struct tx_attenuation_t    // size  2 align  2
{
    uint16_t value;
};

struct db_tx_attenuation_t // size  2 align  2
{
    uint16_t value;
};

struct dbm_tx_power_t      // size  1 align  1
{
    int8_t value;
};

struct antenna_t           // size  1 align  1
{
    uint8_t index;
};

struct db_antenna_signal_t // size  1 align  1
{
    uint8_t value;
};

struct db_antenna_noise_t  // size  1 align  1
{
    uint8_t value;
};

struct rx_flags_t          // size  2 align  2
{
    enum flags_e : uint16_t {
        RSVD = 0x0001,
        FCS_FAILED = 0x0001,
        BADPLCP = 0x0002,
    };

    uint16_t value;
};

struct tx_flags_t          // size  2 align  2
{
    enum flags_e : uint16_t
    {
        E_FLAGS_FAIL = 0x0001,
        E_FLAGS_CTS = 0x0002,
        E_FLAGS_RTS = 0x0004,
        E_FLAGS_NOACK = 0x0008,
        E_FLAGS_FIXEDSN = 0x0010,
        E_FLAGS_NOREORDER = 0x0020
    };

    uint16_t value;
};

struct rts_retries_t       // size  1 align  1
{
    uint8_t value;
};

struct data_retries_t      // size  1 align  1
{
    uint8_t value;
};

struct mcs_t               // size  3 align  1
{
    enum known_e : uint8_t
    {
        E_KNOWN_BW = 0x01,
        E_KNOWN_MCS = 0x02,
        E_KNOWN_GI = 0x04,
        E_KNOWN_HT = 0x08,
        E_KNOWN_FEC = 0x10,
        E_KNOWN_STBC = 0x20,
        E_KNOWN_NESS_KNOWN = 0x40,
        E_KNOWN_NESS_DATA = 0x80
    };

    enum flags_e : uint8_t
    {
        E_FLAG_BW_MASK = 0x03,
        E_FLAG_BW_20 = 0,
        E_FLAG_BW_40 = 1,
        E_FLAG_BW_20L = 2,
        E_FLAG_BW_20U = 3,

        E_FLAG_SGI = 0x04,
        E_FLAG_HT_GF = 0x08,
        E_FLAG_FEC_LDPC = 0x10,

        E_FLAG_STBC_MASK = 0x60,
        E_FLAG_STBC_1 = 0x20,
        E_FLAG_STBC_2 = 0x40,
        E_FLAG_STBC_3 = 0x60,

        E_FLAG_NESS = 0x80
    };

    uint8_t known;
    uint8_t flags;
    uint8_t mcs_index;
};

struct ampdu_status_t      // size  8 align  2
{
    enum flags_e : uint16_t
    {
        E_FLAG_REPORT_ZEROLEN = 0x0001,
        E_FLAG_IS_ZEROLEN = 0x0002,
        E_FLAG_LAST_KNOWN = 0x0004,
        E_FLAG_IS_LAST = 0x0008,
        E_FLAG_DELIM_CRC_ERR = 0x0010,
        E_FLAG_DELIM_CRC_KNOWN = 0x0020,
        E_FLAG_EOF_VALUE = 0x0040,
        E_FLAG_EOF_VALUE_KNOWN = 0x0080
    };

    uint32_t ref_number;
    uint16_t flags;
    uint8_t delimiter_crc;
    uint8_t reserved;
};

struct vht_t               // size 12 align  2
{
    enum known_e : uint16_t 
    {
        E_KNOWN_STBC = 0x0001,
        E_KNOWN_TXOP_PS_NA = 0x0002,
        E_KNOWN_GI = 0x0004,
        E_KNOWN_SGI_NSYM_DIS = 0x0008,
        E_KNOWN_LDPC_EXTRA_OFDM_SYM = 0x0010,
        E_KNOWN_BEAMFORMED = 0x0020,
        E_KNOWN_BANDWIDTH = 0x0040,
        E_KNOWN_GROUP_ID = 0x0080,
        E_KNOWN_PARTIAL_AID = 0x0100
    };

    enum flags_e : uint8_t
    {
        E_FLAG_STBC = 0x01,
        E_FLAG_TXOP_PS_NA = 0x02,
        E_FLAG_SGI = 0x04,
        E_FLAG_SGI_NSYM_M10_9 = 0x08,
        E_FLAG_LDPC_EXTRA_OFDM_SYM = 0x10,
        E_FLAG_BEAMFORMED = 0x20
    };

    enum bw_e : uint8_t
    {
        E_BW_20MHZ,
        E_BW_40MHZ,
        E_BW_40MHZ_SB20_SI0,
        E_BW_40MHZ_SB20_SI1,
        E_BW_80MHZ,
        E_BW_80MHZ_SB40_SI0,
        E_BW_80MHZ_SB40_SI1,
        E_BW_80MHZ_SB20_SI0,
        E_BW_80MHZ_SB20_SI1,
        E_BW_80MHZ_SB20_SI2,
        E_BW_80MHZ_SB20_SI3,
        E_BW_160MHZ,
        E_BW_160MHZ_SB80_SI0,
        E_BW_160MHZ_SB80_SI1,
        E_BW_160MHZ_SB40_SI0,
        E_BW_160MHZ_SB40_SI1,
        E_BW_160MHZ_SB40_SI2,
        E_BW_160MHZ_SB40_SI3,
        E_BW_160MHZ_SB20_SI0,
        E_BW_160MHZ_SB20_SI1,
        E_BW_160MHZ_SB20_SI2,
        E_BW_160MHZ_SB20_SI3,
        E_BW_160MHZ_SB20_SI4,
        E_BW_160MHZ_SB20_SI5,
        E_BW_160MHZ_SB20_SI6,
        E_BW_160MHZ_SB20_SI7
    };

    enum coding_e : uint8_t {
        E_CODING_LDPC_USER0 = 0x01,
        E_CODING_LDPC_USER1 = 0x02,
        E_CODING_LDPC_USER2 = 0x04,
        E_CODING_LDPC_USER3 = 0x08,
    };

    uint16_t known;
    uint8_t flags;
    uint8_t bandwith;
    uint8_t mcs_nss[4];
    uint8_t coding;
    uint8_t group_id;
    uint16_t partial_aid;

    uint8_t mcs_nss_to_mcs(uint8_t value){return value >> 4;}
    uint8_t mcs_nss_to_nss(uint8_t value){return value & 0xf;}
    uint8_t to_mcs_nss(uint8_t mcs, uint8_t nss) {return mcs << 4 | nss;}
};

struct timestamp_t         // size 12 align  8
{
    enum e_unit_pos : uint8_t
    {
        E_UNIT_MASK = 0x0F,
        E_UNIT_MS = 0x00,
        E_UNIT_US = 0x01,
        E_UNIT_NS = 0x02,

        E_POS_MASK = 0xF0,
        E_POS_FIRST_BIT_MPDU = 0x00,
        E_POS_START_PLCP = 0x10,
        E_POS_END_PPDU = 0x20,
        E_POS_END_MPDU = 0x30,
        E_POS_UKNOWN_OOB = 0xF0
    };

    enum flags_e : uint8_t
    {
        E_FLAG_64BIT = 0,
        E_FLAG_32BIT = 1,
        E_FLAG_ACCURACY_KNOWN = 2
    };

    uint64_t timestamp;
    uint16_t accuracy;
    uint8_t unit_pos;
    uint8_t flags;
};

enum E_FIELD_PRESENCE
{
    E_FIELD_PRESENCE_TSFT = 1 << 0,
	E_FIELD_PRESENCE_FLAGS = 1 << 1,
	E_FIELD_PRESENCE_RATE = 1 << 2,
	E_FIELD_PRESENCE_CHANNEL = 1 << 3,
	E_FIELD_PRESENCE_FHSS = 1 << 4,
	E_FIELD_PRESENCE_DBM_ANTSIGNAL = 1 << 5,
	E_FIELD_PRESENCE_DBM_ANTNOISE = 1 << 6,
	E_FIELD_PRESENCE_LOCK_QUALITY = 1 << 7,
	E_FIELD_PRESENCE_TX_ATTENUATION = 1 << 8,
	E_FIELD_PRESENCE_DB_TX_ATTENUATION = 1 << 9,
	E_FIELD_PRESENCE_DBM_TX_POWER = 1 << 10,
	E_FIELD_PRESENCE_ANTENNA = 1 << 11,
	E_FIELD_PRESENCE_DB_ANTSIGNAL = 1 << 12,
	E_FIELD_PRESENCE_DB_ANTNOISE = 1 << 13,
	E_FIELD_PRESENCE_RX_FLAGS = 1 << 14,
	E_FIELD_PRESENCE_TX_FLAGS = 1 << 15,
	E_FIELD_PRESENCE_RTS_RETRIES = 1 << 16,
	E_FIELD_PRESENCE_DATA_RETRIES = 1 << 17,
    // 18
	E_FIELD_PRESENCE_MCS = 1 << 19,
	E_FIELD_PRESENCE_AMPDU_STATUS = 1 << 20,
	E_FIELD_PRESENCE_VHT = 1 << 21,
	E_FIELD_PRESENCE_TIMESTAMP = 1 << 22,
    // 23
    // 24
    // 25
    // 26
    // 27
    // 28
    // 29
    // 30
    // 31
};

class radiotap_t
{
public:
    radiotap_t(uint8_t* buffer)
    {
        if (!buffer)
        {
            return;
        }

        header = (header_t*) buffer;
        rescan();
    }

    size_t size()
    {
        return header->length;
    }

    uint8_t* end()
    {
        return (uint8_t*)header+size();
    }

    void rescan(bool calculate_len = false)
    {
        scan_status = E_STATUS_PARTIAL;
        auto cp  = (uint8_t*)header + sizeof(header);
        process_sequence(&radiotap_t::tsft, E_FIELD_PRESENCE_TSFT, cp);
        process_sequence(&radiotap_t::flags, E_FIELD_PRESENCE_FLAGS, cp);
        process_sequence(&radiotap_t::rate, E_FIELD_PRESENCE_RATE, cp);
        process_sequence(&radiotap_t::channel, E_FIELD_PRESENCE_CHANNEL, cp);
        process_sequence(&radiotap_t::fhss, E_FIELD_PRESENCE_FHSS, cp);
        process_sequence(&radiotap_t::antenna_signal, E_FIELD_PRESENCE_DBM_ANTSIGNAL, cp);
        process_sequence(&radiotap_t::antenna_noise, E_FIELD_PRESENCE_DBM_ANTNOISE, cp);
        process_sequence(&radiotap_t::lock_quality, E_FIELD_PRESENCE_LOCK_QUALITY, cp);
        process_sequence(&radiotap_t::tx_attenuation, E_FIELD_PRESENCE_TX_ATTENUATION, cp);
        process_sequence(&radiotap_t::db_tx_attenuation, E_FIELD_PRESENCE_DB_TX_ATTENUATION, cp);
        process_sequence(&radiotap_t::dbm_tx_power, E_FIELD_PRESENCE_DBM_TX_POWER, cp);
        process_sequence(&radiotap_t::antenna, E_FIELD_PRESENCE_ANTENNA, cp);
        process_sequence(&radiotap_t::db_antenna_signal, E_FIELD_PRESENCE_DB_ANTSIGNAL, cp);
        process_sequence(&radiotap_t::db_antenna_noise, E_FIELD_PRESENCE_DB_ANTNOISE, cp);
        process_sequence(&radiotap_t::rx_flags, E_FIELD_PRESENCE_RX_FLAGS, cp);
        process_sequence(&radiotap_t::tx_flags, E_FIELD_PRESENCE_TX_FLAGS, cp);
        process_sequence(&radiotap_t::rts_retries, E_FIELD_PRESENCE_RTS_RETRIES, cp);
        process_sequence(&radiotap_t::data_retries, E_FIELD_PRESENCE_DATA_RETRIES, cp);
        if ((1 << 18) & header->presence) return;
        process_sequence(&radiotap_t::mcs, E_FIELD_PRESENCE_MCS, cp);
        process_sequence(&radiotap_t::ampdu_status, E_FIELD_PRESENCE_AMPDU_STATUS, cp);
        process_sequence(&radiotap_t::vht, E_FIELD_PRESENCE_VHT, cp);
        process_sequence(&radiotap_t::timestamp, E_FIELD_PRESENCE_TIMESTAMP, cp);
        if ((1 << 23) & (header->presence)) return;
        if ((1 << 24) & (header->presence)) return;
        if ((1 << 25) & (header->presence)) return;
        if ((1 << 26) & (header->presence)) return;
        if ((1 << 27) & (header->presence)) return;
        if ((1 << 28) & (header->presence)) return;
        if ((1 << 29) & (header->presence)) return;
        if ((1 << 30) & (header->presence)) return;
        if ((1 << 31) & (header->presence)) return;

        scan_length = cp - (uint8_t*)header;

        if (calculate_len)
        {
            header->length = scan_length;
        }
        else if (scan_length > header->length)
        {
            scan_status = E_STATUS_FAILED;
            return;
        }
        scan_status = E_STATUS_OK;
    }

    enum status_e {E_STATUS_OK, E_STATUS_PARTIAL, E_STATUS_FAILED};
    status_e scan_status = E_STATUS_FAILED;
    uint16_t scan_length = 0;

    header_t* header = nullptr;
    tsft_t* tsft = nullptr;
    flags_t* flags = nullptr;
    rate_t* rate = nullptr;
    channel_t* channel = nullptr;
    fhss_t* fhss = nullptr;
    antenna_signal_t* antenna_signal = nullptr;
    antenna_noise_t* antenna_noise = nullptr;
    lock_quality_t* lock_quality = nullptr;
    tx_attenuation_t* tx_attenuation = nullptr;
    db_tx_attenuation_t* db_tx_attenuation = nullptr;
    dbm_tx_power_t* dbm_tx_power = nullptr;
    antenna_t* antenna = nullptr;
    db_antenna_signal_t* db_antenna_signal = nullptr;
    db_antenna_noise_t* db_antenna_noise = nullptr;
    rx_flags_t* rx_flags = nullptr;
    tx_flags_t* tx_flags = nullptr;
    rts_retries_t* rts_retries = nullptr;
    data_retries_t* data_retries = nullptr;
    mcs_t* mcs = nullptr;
    ampdu_status_t* ampdu_status = nullptr;
    vht_t* vht = nullptr;
    timestamp_t* timestamp = nullptr;

    template <typename T>
    void process_sequence(T radiotap_t::* m, int bitmask, uint8_t*& cp)
    {
        using U = typename std::remove_pointer<T>::type;
        if (bitmask & header->presence)
        {
            auto field = (cp + (alignof(U) - uintptr_t(cp) % alignof(U)) % alignof(U));
            this->*m = (U*) field;
            cp = field + sizeof(U);
        }
    }
};

} // namespace radiotap
} // namespace winject

#endif // __WINJECT_RADIOTAP_HPP__