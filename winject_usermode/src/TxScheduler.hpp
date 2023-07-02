#ifndef __WINJECTUM_TXSCHEDULER_HPP__
#define __WINJECTUM_TXSCHEDULER_HPP__

#include <memory>
#include <mutex>

#include <bfc/ITimer.hpp>
#include "ITxScheduler.hpp"
#include "ILLC.hpp"
#include "IRRC.hpp"
#include "Logger.hpp"

class TxScheduler : public ITxScheduler
{
public:
    TxScheduler(bfc::ITimer& timer, IRRC& rrc)
        : timer(timer)
        , rrc(rrc)
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
        Logless(*main_logger, TXS_TRC, "TRC | TxSchd | Adding lcid=# to schedulables...", (int)lcid);
        std::unique_lock<std::mutex> lg(this_mutex);
        if (lcid>=llcs.size())
        {
            llcs.resize(lcid+1);
            schedules_info.resize(lcid+1);
            llcs[lcid] = llc;
            schedules_info[lcid] = {llc};
        }
    }

    void remove_llc(lcid_t lcid)
    {
        Logless(*main_logger, TXS_TRC, "TRC | TxSchd | Removing lcid=# from schedulables...", (int)lcid);
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

        frame_info.slot_interval_us = c.slot_interval_us;
        frame_info.fec_type = c.fec_type;
        frame_info.frame_payload_size = c.frame_payload_size;

        Logless(*main_logger, TXS_INF, "INF | TxSchd | Scheduler tick will run in every #us", frame_info.slot_interval_us);
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
    }

private:
    void schedule_tick()
    {
        if (frame_info.slot_interval_us)
        {
            auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();
            int64_t wait_time = 0;
            int64_t diff_time = now - last_tick;
            int64_t error = frame_info.slot_interval_us - diff_time;
            tick_pid_integral += double(error)*0.1;

            last_tick = now;

            if (std::abs(error) > frame_info.slot_interval_us)
            {
                wait_time = frame_info.slot_interval_us;
                tick_pid_integral = 0;
            }
            else
            {
                wait_time = frame_info.slot_interval_us
                    + tick_pid_integral;
            }

            slot_timer_id = timer.schedule(std::chrono::nanoseconds(
                wait_time*1000), 
                [this](){tick();});
        }
    }

    void tick()
    {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch())
                .count();

        schedule_tick();

        std::unique_lock<std::mutex> lg(this_mutex);
        frame_info.slot_number++;

        tx_info_t tx_info{frame_info};
        uint8_t* cursor = buffer + header_size;
        size_t frame_payload_remaining = frame_info.frame_payload_size;

        auto advance_cursor = [&cursor, &frame_payload_remaining](size_t size)
        {
            cursor += size;
            frame_payload_remaining -= size;
        };

        uint8_t *fec_type = cursor;
        *fec_type = frame_info.fec_type;
        advance_cursor (sizeof(*fec_type));

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
            tx_info.out_has_data_loaded = false;
            tx_info.out_allocated = 0;

            llc->on_tx(tx_info);

            advance_cursor (tx_info.out_allocated);

            schedule.has_schedulable = tx_info.out_tx_available;
            schedule.has_data_allocated = tx_info.out_has_data_loaded;

            if (schedule.has_schedulable)
            {
                schedules_cache.emplace_back(&schedule);
                Logless(*main_logger, TXS_TRC,
                    "TRC | TxSchd | has schedule slot=# lcid=# def=# n-dalloc=#",
                    frame_info.slot_number,
                    (int) i,
                    schedule.deficit,
                    tx_info.out_allocated);
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
                if (schedule.has_data_allocated)
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
                advance_cursor(tx_info.out_allocated);

                schedule.has_data_allocated = tx_info.out_has_data_loaded;

                Logless(*main_logger, TXS_TRC,
                    "TRC | TxSchd | has schedule slot=# lcid=# deficit=# alloc=# has_data=#",
                    frame_info.slot_number,
                    (int) i->llc->get_lcid(),
                    schedule.deficit,
                    tx_info.out_allocated,
                    (int) tx_info.out_has_data_loaded);
            }

            if (max_quanta_allocated>frame_info.frame_payload_size || schedulable_count==0)
            {
                break;
            }
        }

        size_t send_size = frame_info.frame_payload_size - frame_payload_remaining;
        if (send_size > 1)
        {
            rrc.perform_tx(send_size);
        }
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

    bfc::ITimer& timer;
    IRRC& rrc;
    frame_info_t frame_info;
    std::optional<int> slot_timer_id;
    int64_t last_tick;
    double tick_pid_integral = 0;
    // @note indexed by lcid
    std::vector<scheduling_ctx_t> schedules_info;
    // @note indexed by lcid
    std::vector<ILLC*> llcs;

    std::vector<scheduling_ctx_t*> schedules_cache;

    uint8_t *buffer = nullptr;
    size_t buffer_size = 0;
    size_t header_size = 0;

    std::mutex this_mutex;
};

#endif // schedules_info
