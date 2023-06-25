#ifndef __WINJECTUM_PDCP_HPP__
#define __WINJECTUM_PDCP_HPP__

#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstring>

#include "IPDCP.hpp"
#include "frame_defs.hpp"

#include "Logger.hpp"

class PDCP : public IPDCP
{
public:
    PDCP(lcid_t lcid, const tx_config_t& tx_config, const rx_config_t& rx_config)
        : lcid(lcid)
        , rx_buffer_ring(1024)
        , tx_config(tx_config)
        , rx_config(rx_config)
    {}

    lcid_t get_attached_lcid()
    {
        return lcid;
    }

    void set_tx_enabled(bool value)
    {
        std::unique_lock<std::shared_mutex> lg(tx_mutex);
        // std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
        is_tx_enabled = value;
        // to_tx_queue.clear();
        current_tx_buffer.clear();
        current_tx_offset = 0;
        lg.unlock();

        Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  | tx_enabled=#",
            (int) lcid,
            (int) is_tx_enabled);
    }

    void set_rx_enabled(bool value)
    {
        std::unique_lock<std::shared_mutex> lg(rx_mutex);
        // std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
        is_rx_enabled = value;
        // to_rx_queue.clear();

        for (auto& el : rx_buffer_ring)
        {
            el.is_completed = false;
        }

        rx_sn_synced = false;

        lg.unlock();

        Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  | rx_enabled=#",
            (int) lcid,
            (int) is_rx_enabled);
    }

    virtual tx_config_t get_tx_confg()
    {
        std::shared_lock<std::shared_mutex> lg(tx_mutex);
        return tx_config;
    }

    virtual void reconfigure(const tx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::shared_mutex> lg(tx_mutex);
            tx_config = config;

            Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  | reconfigure tx:", (int)lcid);
            Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  |   allow_segmentation: #", (int)lcid, (int) tx_config.allow_segmentation);
            Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  |   min_commit_size: #", (int)lcid, tx_config.min_commit_size);

            status = is_tx_enabled;
        }
        set_tx_enabled(status);
    }

    virtual rx_config_t get_rx_confg()
    {
        std::shared_lock<std::shared_mutex> lg(rx_mutex);
        return rx_config;
    }

    virtual void reconfigure(const rx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::shared_mutex> lg(rx_mutex);
            rx_config = config;

            Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  | reconfigure rx:", (int) lcid);
            Logless(*main_logger, Logger::DEBUG, "DBG | PDCP#  |   allow_segmentation: #", (int) lcid, (int) rx_config.allow_segmentation);

            status = is_rx_enabled;
        }
        set_rx_enabled(status);
    }

    void on_tx(tx_info_t& info)
    {
        std::unique_lock<std::shared_mutex> lg(tx_mutex);
        if (!is_tx_enabled)
        {
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
            Logless(*main_logger, Logger::TRACE2, "TR2 | PDCP#  | not enough allocation", (int)lcid);
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
                Logless(*main_logger, Logger::TRACE2, "TR2 | PDCP#  | allocated data sn=# to_tx_queue_sz=# data_sz=#",
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

                Logless(*main_logger, Logger::TRACE2,
                    "TR2 | PDCP#  | allocated data sn=# cur=# cur_sz=# data_sz=#",
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

    void on_rx(rx_info_t& info)
    {
        std::unique_lock<std::shared_mutex> lg(rx_mutex);
        if (!is_rx_enabled)
        {
            return;
        }

        pdcp_t pdcp(info.in_pdu.base, info.in_pdu.size);
        pdcp.iv_size = rx_cipher_iv_size;
        pdcp.hmac_size = rx_hmac_size;
        pdcp.rescan();

        uint8_t *payload = pdcp.payload;

        if (pdcp.get_header_size() >= info.in_pdu.size)
        {
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
                break;
            }

            auto sn = segment.get_SN().value_or(0);

            payload += segment.get_SIZE();
            available_data -= segment.get_SIZE();

            auto update_rx_sn = [this, sn](){
                    if (!rx_config.allow_reordering)
                    {
                        std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
                        to_rx_queue.emplace_back(std::move(rx_buffer_ring[0].buffer));
                        to_rx_queue_cv.notify_one();
                        return;
                    }

                    if (!rx_sn_synced)
                    {
                        rx_sn_synced = true;
                        rx_sn = sn;
                    }

                    auto segment_rx_buffer_idx = rx_buffer_ring_index(sn);
                    auto& segment_rx_buffer_el = rx_buffer_ring[segment_rx_buffer_idx];
                    segment_rx_buffer_el.is_completed = true;

                    while (true)
                    {
                        auto current_rx_buffer_idx = rx_buffer_ring_index(rx_sn);
                        auto& current_rx_buffer_el = rx_buffer_ring[current_rx_buffer_idx];
                        auto& current_rx_buffer = current_rx_buffer_el.buffer;

                        if (current_rx_buffer_el.is_completed)
                        {
                            current_rx_buffer_el.is_completed = false;
                            rx_sn++;
                            std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
                            to_rx_queue.emplace_back(std::move(rx_buffer_ring[0].buffer));
                            to_rx_queue_cv.notify_one();
                        }
                        else
                        {
                            break;
                        }
                    }
                };

            pdcp_segment_offset_t offset = segment.get_OFFSET().value_or(0);
            pdcp_segment_offset_t size = segment.get_payload_size();

            auto current_rx_buffer_idx = rx_buffer_ring_index(sn);
            auto& current_rx_buffer_el = rx_buffer_ring[current_rx_buffer_idx];
            auto& current_rx_buffer = current_rx_buffer_el.buffer;

            if (offset + size > current_rx_buffer.size())
            {
                current_rx_buffer.resize(offset + size);
            }

            // @todo : Implement PDCP encryption
            // @todo : Implement PDCP integrity

            std::memcpy(current_rx_buffer.data() + offset, segment.payload, segment.get_payload_size());

            Logless(*main_logger, Logger::TRACE2,
                "TR2 | PDCP#  | received data sn=# cur=#  data_sz=#",
                (int) lcid,
                (int) sn,
                offset,
                segment.get_payload_size());

            if (segment.is_LAST())
            {
                Logless(*main_logger, Logger::TRACE2,
                    "TR2 | PDCP#  | received data complete data_sz=#",
                    (int) lcid,
                    (int) rx_sn,
                    current_rx_buffer.size());
                update_rx_sn();
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

        Logless(*main_logger, Logger::TRACE2,
            "TR2 | PDCP#  | to tx msg_sz=# queue_sz=#",
            (int) lcid,
            buffer.size(),
            to_tx_queue.size());

        // @todo: Implement PDCP compression
        to_tx_queue.emplace_back(std::move(buffer));
    }

private:
    size_t rx_buffer_ring_index(pdcp_sn_t sn)
    {
        return sn % rx_buffer_ring.size();
    }

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
        buffer_t buffer;
        bool is_completed = false;
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
};

#endif // __WINJECTUM_PDCP_HPP__
