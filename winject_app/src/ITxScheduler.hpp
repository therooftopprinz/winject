#ifndef __WINJECTUM_ITXSCHEDULER_HPP__
#define __WINJECTUM_ITXSCHEDULER_HPP__

#include <functional>
#include <chrono>

struct ITxScheduler
{
    struct buffer_config_t
    {
        uint8_t *buffer = nullptr;
        size_t buffer_size = 0;
        size_t header_size = 0;
    };

    struct frame_scheduling_config_t
    {
        uint64_t slot_interval_us;
        fec_type_e fec_type;
        size_t frame_payload_size = 0;
    };

    struct llc_scheduling_config_t
    {
        lcid_t llcid;
        size_t quanta;
        size_t nd_gpdu_max_size;
    };

    ~ITxScheduler(){}

    virtual void stop() = 0; 
};

#endif // __WINJECTUM_ITXSCHEDULER_HPP__
