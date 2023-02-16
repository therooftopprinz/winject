#ifndef __WINJECTUM_IPDCP_HPP__
#define __WINJECTUM_IPDCP_HPP__

#include "pdu.hpp"
#include "info_defs.hpp"

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

        bool allow_segmentation = false;
        size_t min_commit_size = 1000;
    };

    struct rx_config_t
    {
        std::vector<uint8_t> rx_cipher_key;
        std::vector<uint8_t> rx_integrity_key;
        cipher_algo_e rx_cipher_algorigthm = E_CIPHER_ALGO_NONE;
        integrity_algo_e rx_integrity_algorigthm = E_INT_ALGO_NONE;
        compressiom_alg_e rx_compression_algorigthm = E_COMPRESSION_ALGO_NONE;
        uint8_t tx_compression_level = 0;

        bool allow_segmentation = false;
    };

    virtual ~IPDCP(){} 

    virtual void on_tx(tx_info_t&) = 0;
    virtual void on_rx(rx_info_t&) = 0;
    virtual buffer_t to_rx(uint64_t=-1) = 0;
    virtual void to_tx(buffer_t) = 0;

    // virtual void set_tx_enabled(bool) = 0;
    // virtual void set_rx_enabled(bool) = 0;

    // virtual buffer_t to_rx(bool) = 0;
    // virtual void to_tx(buffer_t) = 0;
};

#endif // __WINJECTUM_IPDCP_HPP__
