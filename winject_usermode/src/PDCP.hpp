#ifndef __WINJECTUM_PDCP_HPP__
#define __WINJECTUM_PDCP_HPP__

#include "IPDCP.hpp"
#include "frame_defs.hpp"
#include <deque>
#include <mutex>
#include <condition_variable>
#include <cstring>

class PDCP : public IPDCP
{
public:
    PDCP(const tx_config_t& tx_config, const rx_config_t& rx_config)
        : tx_config(tx_config)
        , rx_config(rx_config)
    {}

    void set_tx_enabled(bool value)
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        // std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
        is_tx_enabled = value;
        // to_tx_queue.clear();
        current_tx_buffer.clear();
        current_tx_offset = 0;
    }

    void set_rx_enabled(bool value)
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        // std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
        is_tx_enabled = value;
        // to_rx_queue.clear();
        current_rx_buffer.clear();
        current_rx_offset = 0;
    }

    virtual tx_config_t get_tx_confg()
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        return tx_config;
    }

    virtual void reconfigure(const tx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::mutex> lg(tx_mutex);
            tx_config = config;
            status = is_tx_enabled;
        }
        set_tx_enabled(status);
    }

    virtual rx_config_t get_rx_confg()
    {
        std::unique_lock<std::mutex> lg(rx_mutex);
        return rx_config;
    }

    virtual void reconfigure(const rx_config_t& config)
    {
        bool status;
        {
            std::unique_lock<std::mutex> lg(rx_mutex);
            rx_config = config;
            status = is_rx_enabled;
        }
        set_tx_enabled(status);
    }

    void on_tx(tx_info_t& info)
    {
        std::unique_lock<std::mutex> lg(tx_mutex);
        if (!is_tx_enabled)
        {
            return;
        }

        {
            std::unique_lock<std::mutex> lg(to_tx_queue_mutex);
            info.out_tx_available = to_tx_queue.size() || (current_tx_buffer.size() > current_tx_offset);
        }

        pdcp_t pdcp(info.out_pdu.base, info.out_pdu.size);
        pdcp.iv_size = tx_cipher_iv_size;
        pdcp.hmac_size = tx_hmac_size;
        pdcp.rescan();

        if (!info.out_tx_available ||
            info.out_pdu.size == 0 ||
            pdcp.get_header_size() >= info.out_pdu.size)
        {
            return;
        }

        // @note : wait for the allocation to increase
        // current_tx_buffer.size()
        if (tx_config.min_commit_size > info.out_pdu.size)
        {
            return;
        }

        size_t available_for_data = pdcp.pdu_size - pdcp.get_header_size();
        uint8_t* payload = pdcp.payload;
        size_t allocated_segments = 0;

        while (true)
        {
            pdcp_segment_t segment(payload, available_for_data);
            segment.has_offset = tx_config.allow_segmentation;
            segment.rescan();

            if (segment.get_header_size() > available_for_data)
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

                buffer_t pdu = std::move(to_tx_queue.front());

                // @note : wait for the allocation to increase 
                if (pdu.size() > available_for_data)
                {
                    break;
                }
                to_tx_queue.pop_front();
                // @todo: Implement PDCP compression
                std::memcpy(segment.payload, pdu.data(), pdu.size());
                segment.set_payload_size(pdu.size());
            }
            else
            {
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

                if (alloc_size > remaining_size)
                {
                    break;
                }

                // @todo: Implement PDCP compression
                std::memcpy(segment.payload, current_tx_buffer.data() + current_tx_offset, alloc_size);
                current_tx_offset += alloc_size;

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
        std::unique_lock<std::mutex> lg(rx_mutex);
        if (!is_tx_enabled)
        {
            return;
        }

        pdcp_t pdcp(info.in_pdu.base, info.in_pdu.size);
        pdcp.iv_size = rx_cipher_iv_size;
        pdcp.hmac_size = rx_hmac_size;
        pdcp.rescan();

        uint8_t *payload = pdcp.payload;
        size_t available_data = info.in_pdu.size - pdcp.get_header_size();

        while (true)
        {
            pdcp_segment_t segment(payload, available_data);
            segment.has_offset = rx_config.allow_segmentation;
            segment.rescan();
            available_data -= segment.get_header_size();

            if (!segment.has_offset)
            {
                buffer_t buffer;
                buffer.resize(segment.get_payload_size());
                // @todo : Implement PDCP encryption
                // @todo : Implement PDCP integrity
                // @todo : Implement PDCP decompression
                std::memcpy(buffer.data(), segment.payload, segment.get_payload_size());
                std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
                to_rx_queue.emplace_back(std::move(buffer));
                to_rx_queue_cv.notify_one();
            }
            else
            {
                pdcp_segment_offset_t offset = *segment.get_OFFSET();
                pdcp_segment_offset_t size = segment.get_payload_size();

                if (offset + size > current_rx_buffer.size())
                {
                    current_rx_buffer.resize(offset + size);
                }

                // @todo : Implement PDCP encryption
                // @todo : Implement PDCP integrity
                // @todo : Implement PDCP decompression
                std::memcpy(current_rx_buffer.data(), segment.payload, segment.get_payload_size());

                if (rx_sn != segment.get_SN())
                {
                    rx_sn = segment.get_SN();
                    std::unique_lock<std::mutex> lg(to_rx_queue_mutex);
                    to_rx_queue.emplace_back(std::move(current_rx_buffer));
                    to_rx_queue_cv.notify_one();
                }
            }
        }
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
    tx_config_t tx_config;
    rx_config_t rx_config;

    pdcp_sn_t tx_sn = 0;
    size_t rx_sn = -1;
    size_t tx_cipher_iv_size = 0;
    size_t rx_cipher_iv_size = 0;
    size_t tx_hmac_size = 0;
    size_t rx_hmac_size = 0;

    buffer_t current_tx_buffer;
    buffer_t current_rx_buffer;
    size_t   current_tx_offset = 0;
    size_t   current_rx_offset = 0;
    size_t   current_rx_max = 0;

    std::deque<buffer_t> to_tx_queue;
    std::deque<buffer_t> to_rx_queue;

    std::mutex to_tx_queue_mutex;
    std::mutex to_rx_queue_mutex;

    std::condition_variable to_rx_queue_cv;

    bool is_tx_enabled = false;
    bool is_rx_enabled = false;
    std::mutex tx_mutex;
    std::mutex rx_mutex;
};

#endif // __WINJECTUM_PDCP_HPP__
