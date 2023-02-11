#ifndef __WINJECTUM_TXSCHEDULER_HPP__
#define __WINJECTUM_TXSCHEDULER_HPP__

#include <memory>
#include <mutex>

#include "ITimer.hpp"
#include "ITxScheduler.hpp"
#include "ILLC.hpp"
#include "IWIFI.hpp"

class TxScheduler : public ITxScheduler
{
public:
    struct buffer_config_t
    {
        uint8_t *buffer = nullptr;
        size_t buffer_size = 0;
        size_t header_size = 0;
        size_t frame_payload_max_size = 0;
    };

    struct frame_scheduling_config_t
    {
        uint64_t slot_interval_us;
        fec_type_e fec_type;
    };

    struct llc_scheduling_config_t
    {
        lcid_t llcid;
        size_t quanta;
        size_t nd_gpdu_max_size;
    };

    TxScheduler(ITimer& timer, IWIFI& wifi)
        : timer(timer)
        , wifi(wifi)
    {
    }

    ~TxScheduler()
    {
        if (slot_timer_id)
        {
            timer.cancel(*slot_timer_id);
        }
    }

    void add_llc(lcid_t lcid, ILLC* llc)
    {
        std::unique_lock<std::mutex> lg(this_mutex);
        if (lcid>=llcs.size())
        {
            llcs.resize(lcid+1);
            schedules_info.resize(lcid+1);
            llcs[lcid] = llc;
            schedules_info[lcid] = {};
        }
    }

    void remove_llc(lcid_t lcid)
    {
        std::unique_lock<std::mutex> lg(this_mutex);

        llcs[lcid] = nullptr;
        schedules_info[lcid] = {};
    }

    void reconfigure(const frame_scheduling_config_t& c)
    {
        std::unique_lock<std::mutex> lg(this_mutex);
        frame_info.fec_type = c.fec_type;

        if (slot_timer_id)
        {
            timer.cancel(*slot_timer_id);
            slot_timer_id.reset();
        }
        schedule_tick();
    }

    void reconfigure(const llc_scheduling_config_t& c)
    {
        std::unique_lock<std::mutex> lg(this_mutex);
        auto& schedule = schedules_info.at(c.llcid);
        schedule.quanta = c.quanta;
        schedule.nd_gpdu_max_size = c.nd_gpdu_max_size;
    }

    void reconfigure(const buffer_config_t& c)
    {
        buffer = c.buffer;
        buffer_size = c.buffer_size;
        header_size = c.header_size;
        frame_payload_max_size = c.frame_payload_max_size;
    }

private:
    void schedule_tick()
    {
        if (frame_info.slot_interval_us)
        {
            slot_timer_id = timer.schedule(std::chrono::nanoseconds(
                frame_info.slot_interval_us*1000), 
                [this](){tick();});
        }
    }

    void tick()
    {
        std::unique_lock<std::mutex> lg(this_mutex);
        frame_info.slot_number++;

        tx_info_t tx_info{frame_info};
        uint8_t* cursor = buffer + header_size;
        size_t frame_payload_remaining = frame_payload_max_size;

        uint8_t *fec_type = cursor;
        *fec_type = frame_info.fec_type;
        cursor += sizeof(*fec_type);
        frame_payload_remaining -= sizeof(*fec_type);

        // @todo : setup fec structure

        schedules_cache.clear();
        schedules_cache.reserve(llcs.size());

        // @note probing of tx data and non data pdu allocation
        tx_info.in_allow_data = false;
        for (int i=0; i<llcs.size(); i++)
        {
            if (!llcs[i])
            {
                continue;
            }

            auto& llc = llcs[i];
            auto& schedule = schedules_info[i];

            tx_info.out_pdu.base = cursor;
            tx_info.out_pdu.size = std::min(frame_payload_remaining, schedule.nd_gpdu_max_size);
            tx_info.out_tx_available = false;

            llc->on_tx(tx_info);

            cursor += tx_info.out_allocated;
            frame_payload_remaining -= tx_info.out_allocated;

            schedule.llc = llc;
            schedule.has_schedulable = tx_info.out_tx_available;

            if (schedule.has_schedulable)
            {
                schedules_cache.emplace_back(&schedule);
            }
            else
            {
                schedule.deficit = 0;
            }
        }

        tx_info.in_allow_data = true;

        while (frame_payload_remaining)
        {
            for (auto i : schedules_cache)
            {
                i->deficit += i->quanta;
            }

            std::sort(schedules_cache.begin(), schedules_cache.end(),
                [](const scheduling_ctx_t* a, const scheduling_ctx_t* b) -> bool
                {
                    return *a < *b;
                });

            size_t schedulable_count = 0;
            size_t max_quanta_allocated = 0;
            for (auto i : schedules_cache)
            {
                auto& schedule = *i;
                // @note : last schedulable
                if (!schedule.has_data_allocated)
                {
                    break;
                }

                schedulable_count++;

                max_quanta_allocated = std::max(max_quanta_allocated, schedule.deficit);

                tx_info.out_pdu.base = cursor;
                tx_info.out_pdu.size = std::min(frame_payload_remaining, schedule.deficit);
                tx_info.out_tx_available = false;
                tx_info.out_allocated = 0;
                tx_info.out_has_data_loaded = false;
                schedule.llc->on_tx(tx_info);
                cursor += tx_info.out_allocated;
                frame_payload_remaining -= tx_info.out_allocated;
            }

            if (max_quanta_allocated>frame_payload_max_size || schedulable_count==0)
            {
                break;
            }
        }

        size_t send_size = header_size + (frame_payload_max_size - frame_payload_remaining);
        if (send_size)
        {
            wifi.send(buffer, send_size);
        }

        schedule_tick();
    }


    struct scheduling_ctx_t
    {
        ILLC* llc = nullptr;
        size_t deficit = 0;
        size_t quanta = 0;
        size_t nd_gpdu_max_size = 0;
        bool has_schedulable = false; 
        bool has_data_allocated = false;

        bool operator<(const scheduling_ctx_t& other) const
        {
            return deficit*(!has_data_allocated) <
                other.deficit*(!other.has_data_allocated);
        }
    };

    ITimer& timer;
    IWIFI& wifi;
    frame_info_t frame_info;
    std::optional<int> slot_timer_id;

    // @note indexed by lcid
    std::vector<scheduling_ctx_t> schedules_info;
    // @note indexed by lcid
    std::vector<ILLC*> llcs;

    std::vector<scheduling_ctx_t*> schedules_cache;

    uint8_t *buffer = nullptr;
    size_t buffer_size = 0;
    size_t header_size = 0;
    size_t frame_payload_max_size = 0;

    std::mutex this_mutex;
};

#endif // schedules_info
