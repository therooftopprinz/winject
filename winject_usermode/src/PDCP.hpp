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
    PDCP(IRRC& rrc, lcid_t lcid, const tx_config_t& tx_config, const rx_config_t& rx_config)
        : rrc(rrc)
        , lcid(lcid)
        , rx_buffer_ring(8192)
        , tx_config(tx_config)
        , rx_config(rx_config)
    {}

    lcid_t get_attached_lcid()
    {
        return lcid;
    }

    void set_tx_enabled(bool value) override
    {
        std::unique_lock<std::shared_mutex> lg(tx_mutex);
        // std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
        is_tx_enabled = value;
        // to_tx_queue.clear();
        current_tx_buffer.clear();
        current_tx_offset = 0;
        lg.unlock();

        Logless(*main_logger, PDCP_INF, "INF | PDCP#  | tx_enabled=#",
            (int) lcid,
            (int) is_tx_enabled);
    }

    void set_rx_enabled(bool value) override
    {
        std::unique_lock<std::shared_mutex> lg(rx_mutex);
        // std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
        is_rx_enabled = value;
        // to_rx_queue.clear();

        for (auto& el : rx_buffer_ring)
        {
            el.reset();
        }

        rx_sn_synced = false;

        lg.unlock();

        Logless(*main_logger, PDCP_INF, "INF | PDCP#  | rx_enabled=#",
            (int) lcid,
            (int) is_rx_enabled);
    }

    tx_config_t get_tx_confg()
    {
        std::shared_lock<std::shared_mutex> lg(tx_mutex);
        return tx_config;
    }

    void reconfigure(const tx_config_t& config) override
    {
        bool status;
        {
            std::unique_lock<std::shared_mutex> lg(tx_mutex);
            tx_config = config;

            Logless(*main_logger, PDCP_INF, "INF | PDCP#  | reconfigure tx:", (int)lcid);
            Logless(*main_logger, PDCP_INF, "INF | PDCP#  |   allow_segmentation: #", (int)lcid, (int) tx_config.allow_segmentation);
            Logless(*main_logger, PDCP_INF, "INF | PDCP#  |   allow_reordering: #", (int)lcid, (int) tx_config.allow_reordering);
            Logless(*main_logger, PDCP_INF, "INF | PDCP#  |   min_commit_size: #", (int)lcid, tx_config.min_commit_size);

            status = is_tx_enabled;
        }
        set_tx_enabled(status);
    }

    rx_config_t get_rx_confg()
    {
        std::shared_lock<std::shared_mutex> lg(rx_mutex);
        return rx_config;
    }

    void reconfigure(const rx_config_t& config) override
    {
        bool status;
        {
            std::unique_lock<std::shared_mutex> lg(rx_mutex);
            rx_config = config;

            Logless(*main_logger, PDCP_INF, "INF | PDCP#  | reconfigure rx:", (int) lcid);
            Logless(*main_logger, PDCP_INF, "INF | PDCP#  |   allow_segmentation: #", (int) lcid, (int) rx_config.allow_segmentation);
            Logless(*main_logger, PDCP_INF, "INF | PDCP#  |   allow_reordering: #", (int) lcid, (int) rx_config.allow_reordering);

            status = is_rx_enabled;
        }
        set_rx_enabled(status);
    }

    void print_stats()
    {
        LoglessF(*main_logger, LLC_STS, "STS | LLC#   | RAW #,#,#,#,#,#,#", (int) lcid,
            stats.tx_queue_size.load(),
            stats.tx_ignored_pkt.load(),
            stats.rx_reorder_size.load(),
            stats.rx_invalid_pdu.load(),
            stats.rx_ignored_pdu.load(),
            stats.rx_invalid_segment.load(),
            stats.rx_segment_rcvd.load());

        std::atomic<uint64_t> tx_queue_size;
        std::atomic<uint64_t> tx_ignored_pkt;
        std::atomic<uint64_t> rx_reorder_size;
        std::atomic<uint64_t> rx_invalid_pdu;
        std::atomic<uint64_t> rx_ignored_pdu;
        std::atomic<uint64_t> rx_invalid_segment;
        std::atomic<uint64_t> rx_segment_rcvd;

        Logless(*main_logger, PDCP_STS, "STS | PDCP#  | STATS:", (int) lcid);
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   tx_queue_size:      #", (int) lcid, stats.tx_queue_size.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   tx_ignored_pkt:     #", (int) lcid, stats.tx_ignored_pkt.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   rx_reorder_size:    #", (int) lcid, stats.rx_reorder_size.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   rx_invalid_pdu:     #", (int) lcid, stats.rx_invalid_pdu.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   rx_ignored_pdu:     #", (int) lcid, stats.rx_ignored_pdu.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   rx_invalid_segment: #", (int) lcid, stats.rx_invalid_segment.load());
        Logless(*main_logger, PDCP_STS, "STS | PDCP#  |   rx_segment_rcvd:    #", (int) lcid, stats.rx_segment_rcvd.load());
    }

    void on_tx(tx_info_t& info) override
    {
        auto slot_number = info.in_frame_info.slot_number;
        if ( (is_tx_enabled || is_rx_enabled) && last_stats_slot != slot_number &&
            (info.in_frame_info.slot_number & 0x7FF) == 0x7FF)
        {
            last_stats_slot = slot_number;
            print_stats();
        }

        std::unique_lock<std::shared_mutex> lg(tx_mutex);

        if (!is_tx_enabled)
        {
            stats.tx_ignored_pkt.fetch_add(1);
            return;
        }

        {
            std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
            info.out_tx_available = to_tx_queue.size() || (current_tx_buffer.size() > current_tx_offset);
        }

        if (info.out_pdu.size == 0 || !info.out_tx_available)
        {
            return;
        }

        pdcp_t pdcp(info.out_pdu.base, info.out_pdu.size);
        pdcp.iv_size = tx_cipher_iv_size;
        pdcp.hmac_size = tx_hmac_size;
        pdcp.rescan();

        // @note : wait for the allocation to increase
        // current_tx_buffer.size()

        if (tx_config.min_commit_size > info.out_pdu.size ||
            pdcp.get_header_size() >= info.out_pdu.size)
        {
            Logless(*main_logger, PDCP_TRC, "TRC | PDCP#  | not enough allocation", (int)lcid);
            return;
        }

        size_t available_for_data = pdcp.pdu_size - pdcp.get_header_size();
        uint8_t* payload = pdcp.payload;
        size_t allocated_segments = 0;

        while (true)
        {
            pdcp_segment_t segment(payload, available_for_data);
            segment.has_offset = tx_config.allow_segmentation;
            segment.has_sn = tx_config.allow_reordering;
            segment.rescan();

            if (segment.get_header_size() >= available_for_data)
            {
                break;
            }

            available_for_data -= segment.get_header_size();

            segment.set_SN(tx_sn);

            if (!segment.has_offset)
            {
                std::unique_lock<std::mutex> lg(to_tx_queue_mutex);

                if (!to_tx_queue.size())
                {
                    break;
                }

                // @note : wait for the allocation to increase 
                if (to_tx_queue.front().size() > available_for_data)
                {
                    break;
                }

                buffer_t pdu = std::move(to_tx_queue.front());
                stats.tx_queue_size = to_tx_queue.size();
                Logless(*main_logger, PDCP_TRC, "TRC | PDCP#  | allocated data sn=# to_tx_queue_sz=# data_sz=#",
                    (int) lcid,
                    (int) tx_sn,
                    to_tx_queue.size(),
                    pdu.size());

                to_tx_queue.pop_front();
                std::memcpy(segment.payload, pdu.data(), pdu.size());
                segment.set_payload_size(pdu.size());
                segment.set_LAST(true);
            }
            else
            {
                // @note : end of current tx buffer
                if (current_tx_offset >= current_tx_buffer.size())
                {
                    std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
                    if (!to_tx_queue.size())
                    {
                        break;
                    }

                    current_tx_buffer = std::move(to_tx_queue.front());
                    current_tx_offset = 0;
                    to_tx_queue.pop_front();
                }

                size_t remaining_size = current_tx_buffer.size() - current_tx_offset;
                size_t alloc_size = std::min(remaining_size, available_for_data);

                Logless(*main_logger, PDCP_TRC,
                    "TRC | PDCP#  | allocated data sn=# cur=# cur_sz=# data_sz=#",
                    (int)lcid,
                    (int) tx_sn,
                    current_tx_offset,
                    current_tx_buffer.size(),
                    alloc_size);

                std::memcpy(segment.payload, current_tx_buffer.data() + current_tx_offset, alloc_size);
                current_tx_offset += alloc_size;

                segment.set_LAST(current_tx_offset >= current_tx_buffer.size());

                segment.set_OFFSET(current_tx_offset);
                segment.set_payload_size(alloc_size);
            }

            payload += segment.get_SIZE();
            available_for_data -= segment.get_payload_size();
            info.out_allocated += segment.get_SIZE();
            allocated_segments++;
            tx_sn++;

            // @todo: Implement PDCP encryption
            // @todo: Implement PDCP integrity
        }

        if (allocated_segments)
        {
            info.out_allocated += pdcp.get_header_size();
        }

        info.out_tx_available = to_tx_queue.size() || (current_tx_buffer.size() > current_tx_offset);
    }

    void update_rx_sn(pdcp_sn_t sn, bool fast_forward)
    {
        if (!rx_config.allow_reordering)
        {
            auto& current_rx_buffer_el = rx_buffer_ring[0];
            current_rx_buffer_el.reset();
            stats.rx_reorder_size.fetch_sub(1);
            std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
            to_rx_queue.emplace_back(std::move(rx_buffer_ring[0].buffer));
            to_rx_queue_cv.notify_one();
            return;
        }

        while (true)
        {

            pdcp_sn_t max_sn = std::numeric_limits<pdcp_sn_t>::max();
            size_t sn_dist = sn >= rx_sn ? (sn - rx_sn) :
                (max_sn - rx_sn + sn + 1);

            auto current_rx_buffer_idx = rx_buffer_ring_index(rx_sn);
            auto& current_rx_buffer_el = rx_buffer_ring[current_rx_buffer_idx];
            auto& current_rx_buffer = current_rx_buffer_el.buffer;

            if (current_rx_buffer_el.is_complete())
            {
                current_rx_buffer_el.reset();
                stats.rx_reorder_size.fetch_sub(1);
                Logless(*main_logger, PDCP_TRC,
                    "TRC | PDCP#  | transported packet sn=#",
                    (int) lcid,
                    rx_sn);
                
                rx_sn++;
                std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
                to_rx_queue.emplace_back(std::move(current_rx_buffer));
                to_rx_queue_cv.notify_one();
            }
            else if (fast_forward)
            {
                if (current_rx_buffer_el.recvd_offsets.size())
                {
                    stats.rx_reorder_size.fetch_sub(1);
                }

                current_rx_buffer_el.reset();

                Logless(*main_logger, PDCP_TRC,
                    "TRC | PDCP#  | dropped packet sn=#",
                    (int) lcid,
                    rx_sn);

                rx_sn++;
                if (rx_sn == sn || rx_config.max_sn_distance > sn_dist)
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }
    }

    void on_rx(rx_info_t& info) override
    {
        std::unique_lock<std::shared_mutex> lg(rx_mutex);
        if (!is_rx_enabled)
        {
            stats.rx_ignored_pdu.fetch_add(1);
            return;
        }

        pdcp_t pdcp(info.in_pdu.base, info.in_pdu.size);
        pdcp.iv_size = rx_cipher_iv_size;
        pdcp.hmac_size = rx_hmac_size;
        pdcp.rescan();

        uint8_t *payload = pdcp.payload;

        if (pdcp.get_header_size() >= info.in_pdu.size)
        {
            stats.rx_invalid_pdu.fetch_add(1);
            return;
        }

        size_t available_data = info.in_pdu.size - pdcp.get_header_size();

        while (true)
        {
            pdcp_segment_t segment(payload, available_data);
            segment.has_offset = rx_config.allow_segmentation;
            segment.has_sn = rx_config.allow_reordering;
            segment.rescan();

            if (segment.get_header_size() >= available_data)
            {
                if (available_data) stats.rx_invalid_segment.fetch_add(1);
                break;
            }

            stats.rx_segment_rcvd.fetch_add(1);

            pdcp_sn_t sn = segment.get_SN().value_or(0u);

            payload += segment.get_SIZE();
            available_data -= segment.get_SIZE();

            pdcp_segment_offset_t offset = segment.get_OFFSET().value_or(0);
            pdcp_segment_offset_t size = segment.get_payload_size();

            if (!rx_sn_synced)
            {
                rx_sn_synced = true;
                rx_sn = sn;
            }

            pdcp_sn_t max_sn = std::numeric_limits<pdcp_sn_t>::max();
            size_t sn_dist = sn >= rx_sn ? (sn - rx_sn) :
                (max_sn - rx_sn + sn + 1);

            if (sn_dist && (max_sn - sn_dist) <= rx_config.max_sn_distance)
            {
                Logless(*main_logger, PDCP_TRC,
                    "TRC | PDCP#  | Ignored completed (before) sn=#",
                    (int) lcid,
                    sn);
                continue;
            }

            bool fast_forward = false;
            if (sn_dist > rx_config.max_sn_distance)
            {
                Logless(*main_logger, PDCP_ERR,
                    "ERR | PDCP#  | sn out of range! sn=# rx_sn=# distance=#",
                    (int) lcid,
                    sn,
                    rx_sn,
                    sn_dist);

                if (rx_config.allow_rlf)
                {
                    lg.unlock();
                    rrc.on_rlf_rx(lcid);
                    break;
                }

                fast_forward = true;
            }

            auto current_rx_buffer_idx = rx_buffer_ring_index(sn);
            auto& current_rx_buffer_el = rx_buffer_ring[current_rx_buffer_idx];
            auto& current_rx_buffer = current_rx_buffer_el.buffer;

            if (current_rx_buffer_el.is_complete())
            {
                Logless(*main_logger, PDCP_TRC,
                    "TRC | PDCP#  | Ignored completed (after) sn=#",
                    (int) lcid,
                    sn);
                continue;
            }

            if (offset + size > current_rx_buffer.size())
            {
                current_rx_buffer.resize(offset + size);
            }

            // @todo : Implement PDCP encryption
            // @todo : Implement PDCP integrity

            if (segment.is_LAST())
            {
                current_rx_buffer_el.expected_size = offset+size;
                current_rx_buffer_el.has_expected_size = true;
            }

            // Only copy not yet received offset
            if (!current_rx_buffer_el.recvd_offsets.count(offset))
            {
                std::memcpy(current_rx_buffer.data() + offset, segment.payload, size);
                current_rx_buffer_el.recvd_offsets.emplace(offset);
                current_rx_buffer_el.accumulated_size += size;
            }

            Logless(*main_logger, PDCP_TRC,
                "TRC | PDCP#  | segment complete=# sn=# rx_sn=# is_last=# seg_off=# seg_sz=#",
                (int) lcid,
                (int) current_rx_buffer_el.is_complete(),
                sn,
                rx_sn,
                (int) segment.is_LAST(),
                offset,
                size);

            if (current_rx_buffer_el.is_complete())
            {
                stats.rx_reorder_size.fetch_add(1);
                update_rx_sn(sn, fast_forward);
            }
        }
    }

    buffer_t to_rx(uint64_t timeout_us=-1)
    {
        auto pred = [this]() -> bool {
                return to_rx_queue.size();
            };

        std::unique_lock<std::mutex> lg(to_rx_queue_mutex);

        if (to_rx_queue.size())
        {
            auto rv = std::move(to_rx_queue.front());
            to_rx_queue.pop_front();
            // @todo: Implement PDCP compression
            return rv;
        }

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
            // @todo: Implement PDCP compression
            return rv;
        }

        return {};
    }

    void to_tx(buffer_t buffer)
    {
        std::unique_lock<std::mutex> lg(to_tx_queue_mutex);

        if (!is_tx_enabled)
        {
            return;
        }

        Logless(*main_logger, PDCP_TRC,
            "TRC | PDCP#  | to tx msg_sz=# queue_sz=#",
            (int) lcid,
            buffer.size(),
            to_tx_queue.size());

        // @todo: Implement PDCP compression
        to_tx_queue.emplace_back(std::move(buffer));
    }

    const stats_t& get_stats()
    {
        return stats;
    }


private:
    size_t rx_buffer_ring_index(pdcp_sn_t sn)
    {
        return sn % rx_buffer_ring.size();
    }

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
    std::deque<buffer_t> to_rx_queue;

    std::mutex to_tx_queue_mutex;
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
