#include <fstream>

#include <json/json.hpp>
#include "AppRRC.hpp"

#include "Logger.hpp"
const char* Logger::LoggerRef = "LoggerRefXD";
std::unique_ptr<Logger> main_logger;

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
    if (s == "AM") return ILLC::tx_mode_e::E_TX_MODE_AM;
    if (s == "TM") return ILLC::tx_mode_e::E_TX_MODE_TM;
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
    main_logger = std::make_unique<Logger>("main.log");
    main_logger->setLevel(Logger::TRACE2);
    main_logger->logful();

    Logless(*main_logger, Logger::DEBUG, "DBG | main | Reading from config.json");

    std::ifstream f("config.json");
    auto croot = nlohmann::json::parse(f);
    IRRC::config_t rrc_config;
    auto& rrc =  croot.at("rrc_config");
    auto& frame =  rrc.at("frame_config");
    auto& llcs =  rrc.at("llc_configs");
    auto& pdcps =  rrc.at("pdcp_configs");
    auto& app =  rrc.at("app_config");

    Logless(*main_logger, Logger::TRACE, "DBG | main | --- FEC ---");
    for (auto& fec : frame["fec_configs"])
    {
        auto fec_config = IRRC::fec_config_t{
            to_fec_type_e(fec.at("fec_type")),
            fec.at("threshold")};
        rrc_config.fec_configs.emplace_back(fec_config);
        Logless(*main_logger, Logger::TRACE, "DBG | main | fec_config(#,#)",
            (int) fec_config.threshold,
            (int) fec_config.type);
    }

    rrc_config.frame_config.frame_payload_size = frame.at("frame_payload_size");
    rrc_config.frame_config.slot_interval_us = frame.at("slot_interval_us");

    Logless(*main_logger, Logger::DEBUG, "DBG | main | --- FRAME ---");
    Logless(*main_logger, Logger::DEBUG, "DBG | main | frame_config.frame_payload_size: #", rrc_config.frame_config.frame_payload_size);
    Logless(*main_logger, Logger::DEBUG, "DBG | main | frame_config.slot_interval_us: #", rrc_config.frame_config.slot_interval_us);

    Logless(*main_logger, Logger::DEBUG, "DBG | main | --- LLCs ---");
    for (auto& llc : llcs)
    {
        uint8_t lcid = llc.at("lcid");
        auto& llc_config = rrc_config.llc_configs[lcid];
        auto& scheduling_config = rrc_config.scheduling_configs[lcid];

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

        Logless(*main_logger, Logger::DEBUG, "DBG | main | lcid: #", (int) lcid);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   mode: #", (int) llc_config.mode);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   arq_window_size: #", llc_config.arq_window_size);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   max_retx_count: #", llc_config.max_retx_count);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   crc_type: #", (int) llc_config.crc_type);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   nd_gpdu_max_size: #", scheduling_config.nd_gpdu_max_size);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   quanta: #", scheduling_config.quanta);
    }

    Logless(*main_logger, Logger::DEBUG, "DBG | main | --- PDCPs ---");
    for (auto& pdcp : pdcps)
    {
        uint8_t lcid = pdcp.at("lcid");
        auto& pdcp_config = rrc_config.pdcp_configs[lcid];
        pdcp_config.allow_segmentation = pdcp.at("allow_segmentation");
        pdcp_config.allow_reordering = pdcp.at("allow_reordering");
        pdcp_config.min_commit_size = pdcp.at("min_commit_size");

        // pdcp_config.tx_cipher_key;
        // pdcp_config.tx_integrity_key;
        // pdcp_config.tx_cipher_algorigthm;
        // pdcp_config.tx_integrity_algorigthm;
        // pdcp_config.tx_compression_algorigthm;
        // pdcp_config.rx_compression_level;

        auto& ep_config = rrc_config.ep_configs[lcid];
        ep_config.lcid = lcid;
        ep_config.type = pdcp.at("type");

        Logless(*main_logger, Logger::DEBUG, "DBG | main | lcid: #", (int) lcid);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   allow_segmentation: #", (int) pdcp_config.allow_segmentation);
        Logless(*main_logger, Logger::DEBUG, "DBG | main |   min_commit_size: #", pdcp_config.min_commit_size);

        Logless(*main_logger, Logger::DEBUG, "DBG | main |   type: #", ep_config.type.c_str());

        if (ep_config.type == "udp")
        {
            ep_config.address1 = pdcp.at("tx_address");
            ep_config.address2 = pdcp.at("rx_address");
            Logless(*main_logger, Logger::DEBUG, "DBG | main |   rx_address: #", ep_config.address1.c_str());
            Logless(*main_logger, Logger::DEBUG, "DBG | main |   tx_address: #", ep_config.address2.c_str());
        }
    }

    if (app.count("wifi_device"))
    {
        rrc_config.app_config.wifi_device = app.at("wifi_device");
    }
    else if (app.count("dual_wifi"))
    {
        auto& dual_wifi = app.at("dual_wifi");
        rrc_config.app_config.wifi_device = dual_wifi.at("tx");
        rrc_config.app_config.wifi_device2 = dual_wifi.at("rx");
    }
    else
    {
        auto& wifi_over_udp = app.at("wifi_over_udp");
        rrc_config.app_config.woudp_tx = wifi_over_udp.at("tx_address");
        rrc_config.app_config.woudp_rx = wifi_over_udp.at("rx_address");
    }

    rrc_config.app_config.udp_console = app.at("udp_console");
    rrc_config.app_config.hwsrc = app.at("hwsrc");
    rrc_config.app_config.hwdst = app.at("hwdst");

    Logless(*main_logger, Logger::DEBUG, "DBG | main | --- APP ---");
    Logless(*main_logger, Logger::DEBUG, "DBG | main | wifi_device: #", rrc_config.app_config.wifi_device.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | wifi_device2: #", rrc_config.app_config.wifi_device2.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | woudp_tx: #", rrc_config.app_config.woudp_tx.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | woudp_rx: #", rrc_config.app_config.woudp_rx.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | udp_console #", rrc_config.app_config.udp_console.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | hwsrc: #", rrc_config.app_config.hwsrc.c_str());
    Logless(*main_logger, Logger::DEBUG, "DBG | main | hwdst: #", rrc_config.app_config.hwdst.c_str());

    AppRRC app_rrc{rrc_config};
    app_rrc.run();
    return 0;
}
