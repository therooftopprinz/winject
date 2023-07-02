#ifndef __WINJECTUM_ILLC_HPP__
#define __WINJECTUM_ILLC_HPP__

#include <atomic>

#include "pdu.hpp"
#include "frame_defs.hpp"
#include "info_defs.hpp"

struct ILLC
{
    enum crc_type_e
    {
        E_CRC_TYPE_NONE,
        E_CRC_TYPE_CRC32_04C11DB7
    };

    enum tx_mode_e
    {
        E_TX_MODE_TM,
        E_TX_MODE_AM
    };

    struct tx_config_t
    {
        tx_mode_e mode = E_TX_MODE_TM;
        size_t arq_window_size = 0;
        size_t max_retx_count = 0;
        crc_type_e crc_type = E_CRC_TYPE_NONE;
        bool allow_rlf = false;
    };

    struct rx_config_t
    {
        crc_type_e crc_type = E_CRC_TYPE_NONE;
        tx_mode_e mode = E_TX_MODE_TM;
    };

    struct stats_t
    {
        std::atomic<uint64_t> pkt_sent;
        std::atomic<uint64_t> pkt_resent;
        std::atomic<uint64_t> pkt_recv;
        std::atomic<uint64_t> bytes_sent;
        std::atomic<uint64_t> bytes_resent;
        std::atomic<uint64_t> bytes_recv;
    };

    virtual ~ILLC(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;

    virtual void set_tx_enabled(bool) = 0;
    virtual void set_rx_enabled(bool) = 0;
    virtual tx_config_t get_tx_confg() = 0;
    virtual rx_config_t get_rx_confg() = 0;
    virtual void reconfigure(const tx_config_t&) = 0;
    virtual void reconfigure(const rx_config_t&) = 0;
    virtual lcid_t get_lcid() = 0;

    virtual const stats_t& get_stats() = 0;
};

#endif // __WINJECTUM_ILLC_HPP__
