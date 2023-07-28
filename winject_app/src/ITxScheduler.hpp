/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __WINJECTUM_ITXSCHEDULER_HPP__
#define __WINJECTUM_ITXSCHEDULER_HPP__

#include <functional>
#include <chrono>

#include <bfc/IMetric.hpp>

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

    struct stats_t
    {
        bfc::IMetric* tick_error = nullptr;
        bfc::IMetric* tick_error_avg46 = nullptr;
        bfc::IMetric* tick = nullptr;
    };

    ~ITxScheduler(){}

    virtual void stop() = 0;
};

#endif // __WINJECTUM_ITXSCHEDULER_HPP__
