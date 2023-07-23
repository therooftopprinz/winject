#ifndef __WINJECTUM_IRRC_HPP__
#define __WINJECTUM_IRRC_HPP__

#include <map>
#include <string>

#include <bfc/IMetric.hpp>

#include "frame_defs.hpp"

#include "IPDCP.hpp"
#include "ILLC.hpp"
#include "IEndPoint.hpp"
#include "ITxScheduler.hpp"

#include "interface/rrc.hpp"

struct IRRC
{
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

    struct fec_config_t
    {
        fec_type_e type;
        uint8_t threshold;
    };

    struct app_config_t
    {
        std::string udp_console;
        std::string wifi_device;
        std::string wifi_device2;
        std::string woudp_tx;
        std::string woudp_rx;
    };

    struct frame_config_t
    {
        uint64_t slot_interval_us = 0;
        size_t frame_payload_size = 0;
        std::string hwsrc;
        std::string hwdst;
    };

    struct config_t
    {
        frame_config_t frame_config;
        std::vector<fec_config_t> fec_configs;
        std::map<uint8_t, ILLC::tx_config_t> llc_tx_configs;
        std::map<uint8_t, ILLC::rx_config_t> llc_rx_configs;
        std::map<uint8_t, ITxScheduler::llc_scheduling_config_t> scheduling_configs;
        std::map<uint8_t, IPDCP::tx_config_t> pdcp_configs;
        std::map<uint8_t, IEndPoint::config_t> ep_configs;
        app_config_t app_config;
    };

    struct stats_t
    {
        bfc::IMetric* rx_antenna_dbm_avg46 = nullptr;
    };

    virtual ~IRRC(){};
    virtual void on_rlf(lcid_t) = 0;
    virtual void on_init(lcid_t) = 0;
    virtual uint8_t allocate_req_id() = 0;
    virtual void send_rrc(const RRC& rrc) = 0;
    virtual void stop() = 0;

    virtual void perform_tx(size_t) = 0;
};

#endif // __WINJECTUM_IRRC_HPP__
