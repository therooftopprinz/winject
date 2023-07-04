#ifndef __WINJECTUM_PDCP_HPP__
#define __WINJECTUM_PDCP_HPP__

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstring>
#include <unordered_set>

#include "IRRC.hpp"
#include "IPDCP.hpp"
#include "frame_defs.hpp"

#include "Logger.hpp"

class PDCP : public IPDCP
{
public:
    PDCP(IRRC& rrc, lcid_t lcid, const tx_config_t& tx_config, const rx_config_t& rx_config);
    lcid_t get_attached_lcid();

    void set_tx_enabled(bool value) override;
    void set_rx_enabled(bool value) override;

    void reconfigure(const tx_config_t& config) override;
    void reconfigure(const rx_config_t& config) override;

    void on_tx(tx_info_t& info) override;
    void on_rx(rx_info_t& info) override;

    buffer_t to_rx(uint64_t timeout_us=-1) override;
    bool to_tx(buffer_t buffer) override;

    const stats_t& get_stats() override;

private:
    size_t rx_buffer_ring_index(pdcp_sn_t sn);
    void print_stats();
    void update_rx_sn(pdcp_sn_t sn, bool fast_forward);

    IRRC& rrc;
    lcid_t lcid;
    tx_config_t tx_config;
    rx_config_t rx_config;

    pdcp_sn_t tx_sn = 0;
    pdcp_sn_t rx_sn = 0;
    bool rx_sn_synced = false;
    size_t tx_cipher_iv_size = 0;
    size_t rx_cipher_iv_size = 0;
    size_t tx_hmac_size = 0;
    size_t rx_hmac_size = 0;

    buffer_t current_tx_buffer;
    size_t   current_tx_offset = 0;
    struct rx_buffer_info_t
    {
        // pdcp_sn_t sn;
        bool has_expected_size = false;
        size_t accumulated_size = 0;
        size_t expected_size = 0;
        std::unordered_set<size_t> recvd_offsets;
        buffer_t buffer;

        bool is_complete()
        {
            return has_expected_size &&
                expected_size == accumulated_size;
        }

        bool has_data()
        {
            return recvd_offsets.size();
        }

        void reset()
        {
            has_expected_size = false;
            accumulated_size = 0;
            expected_size = 0;
            recvd_offsets.clear();
        }
    };

    std::vector<rx_buffer_info_t> rx_buffer_ring;

    std::deque<buffer_t> to_tx_queue;
    std::mutex to_tx_queue_mutex;
    std::condition_variable to_tx_queue_cv;
    
    std::deque<buffer_t> to_rx_queue;
    std::mutex to_rx_queue_mutex;

    std::condition_variable to_rx_queue_cv;

    bool is_tx_enabled = false;
    bool is_rx_enabled = false;
    std::shared_mutex tx_mutex;
    std::shared_mutex rx_mutex;

    stats_t stats;
    uint64_t last_stats_slot = -1;
};

#endif // __WINJECTUM_PDCP_HPP__
