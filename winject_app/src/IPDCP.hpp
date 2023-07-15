#ifndef __WINJECTUM_IPDCP_HPP__
#define __WINJECTUM_IPDCP_HPP__

#include <atomic>

#include "pdu.hpp"
#include "info_defs.hpp"
#include "frame_defs.hpp"

struct IPDCP
{
    enum cipher_algo_e
    {
        E_CIPHER_ALGO_NONE,
        E_CIPHER_ALGO_AES128_CBC
    };

    enum integrity_algo_e
    {
        E_INT_ALGO_NONE,
        E_INT_ALGO_HMAC_SHA1
    };

    enum compressiom_alg_e
    {
        E_COMPRESSION_ALGO_NONE,
        E_COMPRESSION_ALGO_LZ77_HUFFMAN
    };

    struct tx_config_t
    {
        std::vector<uint8_t> tx_cipher_key;
        std::vector<uint8_t> tx_integrity_key;
        cipher_algo_e tx_cipher_algorigthm = E_CIPHER_ALGO_NONE;
        integrity_algo_e tx_integrity_algorigthm = E_INT_ALGO_NONE;
        compressiom_alg_e tx_compression_algorigthm = E_COMPRESSION_ALGO_NONE;
        uint8_t rx_compression_level = 0;

        bool allow_rlf = false;
        bool allow_segmentation = false;
        bool allow_reordering = false;
        bool auto_init_on_tx = false;
        size_t max_sn_distance = 20;
        size_t min_commit_size = 1000;
        size_t max_tx_queue_size = 512;
    };

    struct rx_config_t
    {
        std::vector<uint8_t> rx_cipher_key;
        std::vector<uint8_t> rx_integrity_key;
        cipher_algo_e rx_cipher_algorigthm = E_CIPHER_ALGO_NONE;
        integrity_algo_e rx_integrity_algorigthm = E_INT_ALGO_NONE;
        compressiom_alg_e rx_compression_algorigthm = E_COMPRESSION_ALGO_NONE;
        uint8_t tx_compression_level = 0;

        bool allow_rlf = false;
        bool allow_segmentation = false;
        bool allow_reordering = false;
        size_t max_sn_distance = 20;
    };

    struct stats_t
    {
        std::atomic<uint64_t> tx_queue_size;
        std::atomic<uint64_t> rx_reorder_size;
        std::atomic<uint64_t> rx_invalid_pdu;
        std::atomic<uint64_t> rx_ignored_pdu;
        std::atomic<uint64_t> rx_invalid_segment;
        std::atomic<uint64_t> rx_segment_rcvd;
        
    };

    virtual ~IPDCP(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;
    virtual buffer_t to_rx(uint64_t=-1) = 0;
    virtual bool to_tx(buffer_t) = 0;

    virtual lcid_t get_attached_lcid() = 0;

    virtual void set_tx_enabled(bool) = 0;
    virtual void set_rx_enabled(bool) = 0;

    virtual void reconfigure(const tx_config_t& config) = 0;
    virtual void reconfigure(const rx_config_t& config) = 0;

    virtual tx_config_t get_tx_config() = 0;
    virtual rx_config_t get_rx_config() = 0;

    virtual const stats_t& get_stats() = 0;
};

#endif // __WINJECTUM_IPDCP_HPP__
