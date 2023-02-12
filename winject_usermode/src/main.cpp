#include <fstream>

#include <json/json.hpp>
#include "AppRRC.hpp"

fec_type_e to_fec_type_e(std::string s)
{
    if (s == "RS(255,239)") return fec_type_e::E_FEC_TYPE_RS_255_239;
    if (s == "RS(255,223)") return fec_type_e::E_FEC_TYPE_RS_255_223;
    if (s == "RS(255,191)") return fec_type_e::E_FEC_TYPE_RS_255_191;
    if (s == "RS(255,127)") return fec_type_e::E_FEC_TYPE_RS_255_127;
    if (s == "NONE") return fec_type_e::E_FEC_TYPE_NONE;
    throw std::runtime_error("to_fec_type_e: invalid");
}

ILLC::tx_mode_e to_tx_mode(std::string s)
{
    if (s == "RS(255,239)") return ILLC::tx_mode_e::E_TX_MODE_AM;
    if (s == "RS(255,239)") return ILLC::tx_mode_e::E_TX_MODE_UM;
    throw std::runtime_error("to_tx_mode: invalid");
}

ILLC::crc_type_e to_crc_type(std::string s)
{
    if (s == "CRC32(04C11DB7)") return ILLC::crc_type_e::E_CRC_TYPE_CRC32_04C11DB7;
    if (s == "NONE") return ILLC::crc_type_e::E_CRC_TYPE_NONE;
    throw std::runtime_error("to_crc_type: invalid");
}

int main()
{
    std::ifstream f("config.json");
    auto croot = nlohmann::json::parse(f);
    IRRC::config_t rrc_config;
    auto& rrc =  croot.at("rrc_config");
    auto& frame =  rrc.at("frame_config");
    auto& llcs =  rrc.at("llc_configs");
    auto& pdcps =  rrc.at("pdcp_configs");
    auto& app =  rrc.at("app_config");

    for (auto& fec : frame["fec_configs"])
    {
        rrc_config.fec_configs.emplace_back(IRRC::fec_config_t{
            to_fec_type_e(fec.at("fec_type")),
            fec.at("threshold")
        });
    }

    rrc_config.frame_info.frame_size = frame.at("frame_size");
    rrc_config.frame_info.slot_interval_us = frame.at("slot_interval_us");

    for (auto& llc : frame["llc_configs"])
    {
        auto& llc_config = rrc_config.llc_configs[llc.at("lcid")];
        auto& scheduling_config = rrc_config.scheduling_configs[llc.at("lcid")];

        llc_config.mode = to_tx_mode(llc.at("tx_mode"));
        auto& scheduling_config_j = llc.at("scheduling_config");
        scheduling_config.nd_gpdu_max_size = scheduling_config_j.at("nd_pdu_size");
        scheduling_config.quanta = scheduling_config_j.at("quanta");
        if (ILLC::tx_mode_e::E_TX_MODE_AM == llc_config.mode)
        {
            auto& tx_config = llc.at("tx_config");
            llc_config.arq_window_size = tx_config.at("arq_window_size");
            llc_config.max_retx_count = tx_config.at("max_retx_count");
        }
        else if (llc.count("tx_config"))
        {
            auto& tx_config = llc.at("tx_config");
            llc_config.crc_type = to_crc_type(tx_config.at("crc_type"));
        }
    }


    for (auto& pdcp : frame["pdcp_configs"])
    {
        auto& pdcp_config = rrc_config.pdcp_configs[pdcp.at("lcid")];
        pdcp_config.allow_segmentation = pdcp.at("allow_segmentation");
        pdcp_config.min_commit_size = pdcp.at("min_commit_size");
        // pdcp_config.tx_cipher_key;
        // pdcp_config.tx_integrity_key;
        // pdcp_config.tx_cipher_algorigthm;
        // pdcp_config.tx_integrity_algorigthm;
        // pdcp_config.tx_compression_algorigthm;
        // pdcp_config.rx_compression_level;
    }

    rrc_config.app_config.wifi_device = app.at("wifi_device");
    rrc_config.app_config.udp_console_host = app.at("udp_console_host");
    rrc_config.app_config.udp_console_port = app.at("udp_console_port");

    AppRRC app_rrc{rrc_config};
    app_rrc.run();
    return;
}
