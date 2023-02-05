#ifndef __WINJECTUM_PDCP_HPP__
#define __WINJECTUM_PDCP_HPP__

#include "IPDCP.hpp"
#include "frame_defs.hpp"
#include <deque>
#include <mutex>
#include <condition_variable>

class PDCP : public IPDCP
{
public:
    PDCP()
    {}

    void on_tx(tx_info_t& info)
    {
        info.tx_available = to_tx_queue.size() || current_tx_buffer.size();

        pdcp_t pdcp(info.out_pdu.base, info.out_pdu.size);
        pdcp.iv_ext_size = tx_cipher_iv_size;
        pdcp.hmac_size = tx_cipher_iv_size;

        if (!info.tx_available ||
            info.out_pdu.size == 0 ||
            pdcp.get_header_size() >= info.out_pdu.size)
        {
            return;
        }

        bool below_min_commit_size = min_commit_size > info.out_pdu.size;
        // @todo include LLC header and frame header in this calculation
        bool commit_above_current_mtu = min_commit_size > info.in_frame_info.max_frame_payload;

        // @note Wait for the allocation to increase
        if (below_min_commit_size && !commit_above_current_mtu)
        {
            return;
        }

        size_t available_for_data = pdcp.pdu_size - pdcp.get_header_size();
        pdcp.initialize();
        *(pdcp.sn) = tx_sn++;
        // @todo: Implement PDCP compression
        // @todo: Implement PDCP encryption
        // @todo: Implement PDCP integrity
        
    }

    void on_rx(rx_info_t&)
    {

    }

    void on_tx_rlf()
    {
        std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
    }

    void on_rx_rlf()
    {
        std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
    }


    buffer_t to_rx(uint64_t timeout_us=-1)
    {
        auto pred = [this]() -> bool {
                // return to_rx.size();
                return to_rx_queue.size();
            };

        std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
        if (timeout_us==-1)
        {
            to_rx_queue_cv.wait(lg, pred);
        }
        else
        {
            to_rx_queue_cv.wait_for(lg,
                std::chrono::microseconds(timeout_us), pred);
        }
        if (to_rx_queue.size())
        {
            auto rv = std::move(to_rx_queue.front());
            to_rx_queue.pop_front();
            return rv;
        }
        return {};
    }

    void to_tx(buffer_t buffer)
    {
        std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
        to_tx_queue.emplace_back(std::move(buffer));
    }


private:
    bool is_framing = true;
    size_t min_commit_size = 1000;
    bool commit_small_pdu = true;

    enum cipher_alg_e {
            E_CIPHER_ALG_NONE,
            E_CIPHER_ALG_AES128_CBC
        };
    enum integrity_alg_e {
            E_INT_ALG_NONE,
            E_INT_ALG_HMAC_SHA1
        };
    enum compressiom_alg_e {
            E_COMPRESSION_NONE,
            E_COMPRESSION_ALG_LZ77_HUFFMAN
        };

    pdcp_sn_t tx_sn = 0;
    pdcp_sn_t rx_sn = 0;
    size_t tx_cipher_iv_size = 0;
    size_t rx_cipher_iv_size = 0;
    size_t tx_hmac_size = 0;
    size_t rx_hmac_size = 0;
    std::vector<uint8_t> tx_cipher_key;
    std::vector<uint8_t> rx_cipher_key;
    std::vector<uint8_t> tx_integrity_key;
    std::vector<uint8_t> rx_integrity_key;
    cipher_alg_e tx_cipher_algorigthm;
    cipher_alg_e rx_cipher_algorigthm;
    integrity_alg_e tx_integrity_algorigthm;
    integrity_alg_e tx_integrity_algorigthm;
    compressiom_alg_e rx_compression_algorigthm;
    compressiom_alg_e tx_compression_algorigthm;
    uint8_t tx_compression_level;
    uint8_t rx_compression_level;

    buffer_t current_tx_buffer;
    buffer_t current_rx_buffer;
    size_t   current_tx_offset;
    size_t   current_rx_offset;
    size_t   current_rx_max;

    std::deque<buffer_t> to_tx_queue;
    std::deque<buffer_t> to_rx_queue;

    std::mutex to_tx_queue_mutex;
    std::mutex to_rx_queue_mutex;

    std::condition_variable to_rx_queue_cv;
};

#endif // __WINJECTUM_PDCP_HPP__
