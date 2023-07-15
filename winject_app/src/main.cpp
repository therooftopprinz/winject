#include <fstream>

#include <json/json.hpp>
#include "AppRRC.hpp"

#include "Logger.hpp"

constexpr static size_t DEFAULT_ND_PDU_SIZE = 64;
constexpr static size_t DEFAULT_QUANTA = 256;
constexpr static size_t DEFAULT_FRAME_PAYLOAD_SIZE = 1450;
constexpr static size_t DEFAULT_SLOT_INTERVAL_US = 500;
constexpr static size_t DEFAULT_ARQ_WINDOW_SIZE = 25;
constexpr static size_t DEFAULT_MAX_RETX_COUNT = 50;
const static std::string DEFAULT_CRC_TYPE = "NONE";
constexpr static bool DEFAULT_ALLOW_RLF = true;
constexpr static bool DEFAULT_ALLOW_SEGMENTATION = false;
constexpr static bool DEFAULT_ALLOW_REORDERING = false;
constexpr static size_t DEFAULT_MIN_COMMIT_SIZE = 0;
constexpr static size_t DEFAULT_MAX_SN_DISTANCE = 1000;
constexpr static bool DEFAULT_AUTO_INIT_ON_TX = false;
constexpr static bool DEFAULT_AUTO_INIT_ON_RX = false;


template<> 
const char* LoggerType::LoggerRef = "LoggerRefXD";
std::unique_ptr<LoggerType> main_logger;

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

template <typename T>
T value_or(const nlohmann::json& json, const std::string& key, T value)
{
    T rv = value;
    if (json.contains(key))
    {
        rv = json.at(key);
    }
    return rv;
}

int main()
{
    main_logger = std::make_unique<LoggerType>("main.log");
    main_logger->logful();

    for (auto i=0u; i<app_logbit_e::MAX; i++)
    {
        main_logger->set_logbit(true, i);
    }
    main_logger->set_logbit(false, PDCP_STS);
    main_logger->set_logbit(false, LLC_STS);

    Logless(*main_logger, MAN_INF, "INF | main | Reading from config.json");

    std::ifstream f("config.json");
    auto croot = nlohmann::json::parse(f);
    IRRC::config_t rrc_config;
    auto& rrc =  croot.at("rrc_config");
    auto& frame =  rrc.at("frame_config");
    auto& lc_configs =  rrc.at("lc_configs");
    auto& app =  rrc.at("app_config");

    Logless(*main_logger, MAN_INF, "INF | main | --- FEC ---");
    for (auto& fec : frame["fec_configs"])
    {
        auto fec_config = IRRC::fec_config_t{
            to_fec_type_e(fec.at("fec_type")),
            fec.at("threshold")};
        // @todo : validate fec_type enum
        rrc_config.fec_configs.emplace_back(fec_config);
        Logless(*main_logger, MAN_INF, "INF | main | fec_config(#,#)",
            (int) fec_config.threshold,
            (int) fec_config.type);
    }

    rrc_config.frame_config.frame_payload_size = value_or(frame, "frame_payload_size", DEFAULT_FRAME_PAYLOAD_SIZE);
    rrc_config.frame_config.slot_interval_us   = value_or(frame, "slot_interval_us", DEFAULT_SLOT_INTERVAL_US);
    rrc_config.frame_config.hwsrc = frame.at("hwsrc");
    rrc_config.frame_config.hwdst = frame.at("hwdst");

    Logless(*main_logger, MAN_INF, "INF | main | --- FRAME ---");
    Logless(*main_logger, MAN_INF, "INF | main | frame_config.frame_payload_size: #", rrc_config.frame_config.frame_payload_size);
    Logless(*main_logger, MAN_INF, "INF | main | frame_config.slot_interval_us: #", rrc_config.frame_config.slot_interval_us);
    Logless(*main_logger, MAN_INF, "INF | main | frame_config.hwsrc: #", rrc_config.frame_config.hwsrc.c_str());
    Logless(*main_logger, MAN_INF, "INF | main | frame_config.hwdst: #", rrc_config.frame_config.hwdst.c_str());



    Logless(*main_logger, MAN_INF, "INF | main | --- Logical Channels ---");
    for (auto& lc : lc_configs)
    {
        auto& llc =  lc.at("llc_config");
        auto& pdcp =  lc.at("pdcp_config");

        uint8_t lcid = lc.at("lcid");

        // LLC
        auto& llc_tx_config = rrc_config.llc_tx_configs[lcid];
        auto& llc_rx_config = rrc_config.llc_rx_configs[lcid];
        auto& scheduling_config = rrc_config.scheduling_configs[lcid];

        llc_tx_config.mode = to_tx_mode(llc.at("tx_mode"));

        auto& scheduling_config_j = llc.at("scheduling_config");
        scheduling_config.nd_gpdu_max_size = value_or(scheduling_config_j, "nd_pdu_size", DEFAULT_ND_PDU_SIZE);
        scheduling_config.quanta = value_or(scheduling_config_j, "quanta", DEFAULT_QUANTA);
        if (ILLC::tx_mode_e::E_TX_MODE_AM == llc_tx_config.mode)
        {
            auto& tx_config = llc.at("am_tx_config");
            llc_tx_config.allow_rlf = value_or(tx_config, "allow_rlf", DEFAULT_ALLOW_RLF);
            llc_tx_config.arq_window_size = value_or(tx_config, "arq_window_size", DEFAULT_ARQ_WINDOW_SIZE);
            llc_tx_config.max_retx_count = value_or(tx_config, "max_retx_count", DEFAULT_MAX_RETX_COUNT);
        }

        auto& common_tx_config = llc.at("common_tx_config");
        llc_tx_config.crc_type = to_crc_type(value_or(common_tx_config, "crc_type", DEFAULT_CRC_TYPE));

        llc_rx_config.auto_init_on_rx = value_or(llc, "auto_init_on_rx", DEFAULT_AUTO_INIT_ON_RX);

        llc_rx_config.mode = llc_rx_config.mode;
        llc_rx_config.crc_type = llc_rx_config.crc_type;

        auto& pdcp_config = rrc_config.pdcp_configs[lcid];

        auto& pdcp_tx_config = pdcp.at("tx_config");
        pdcp_config.allow_rlf = value_or(pdcp_tx_config,"allow_rlf", DEFAULT_ALLOW_RLF);
        pdcp_config.allow_segmentation = value_or(pdcp_tx_config,"allow_segmentation", DEFAULT_ALLOW_SEGMENTATION);
        pdcp_config.allow_reordering = value_or(pdcp_tx_config,"allow_reordering", DEFAULT_ALLOW_REORDERING);
        pdcp_config.min_commit_size = value_or(pdcp_tx_config,"min_commit_size", DEFAULT_MIN_COMMIT_SIZE);
        pdcp_config.max_sn_distance = value_or(pdcp_tx_config,"max_sn_distance", DEFAULT_MAX_SN_DISTANCE);
        pdcp_config.auto_init_on_tx = value_or(pdcp_tx_config,"auto_init_on_tx", DEFAULT_AUTO_INIT_ON_TX);

        auto& endpoint_config = pdcp.at("endpoint_config");
        auto& ep_config = rrc_config.ep_configs[lcid];
        ep_config.lcid = lcid;
        ep_config.type = endpoint_config.at("type");

        Logless(*main_logger, MAN_INF, "INF | main | lcid: #", (int) lcid);
        Logless(*main_logger, MAN_INF, "INF | main |   mode: #", (int) llc_tx_config.mode);
        if (llc_tx_config.mode == ILLC::E_TX_MODE_AM)
        {
        Logless(*main_logger, MAN_INF, "INF | main |   arq_window_size: #", llc_tx_config.arq_window_size);
        Logless(*main_logger, MAN_INF, "INF | main |   max_retx_count: #", llc_tx_config.max_retx_count);
        }
        Logless(*main_logger, MAN_INF, "INF | main |   nd_gpdu_max_size: #", scheduling_config.nd_gpdu_max_size);
        Logless(*main_logger, MAN_INF, "INF | main |   quanta: #", scheduling_config.quanta);
        Logless(*main_logger, MAN_INF, "INF | main |   crc_type: #", (int) llc_tx_config.crc_type);
        Logless(*main_logger, MAN_INF, "INF | main |   auto_init_on_rx: #", (int) llc_rx_config.auto_init_on_rx);
        Logless(*main_logger, MAN_INF, "INF | main |   auto_init_on_tx: #", (int) pdcp_config.auto_init_on_tx);
        Logless(*main_logger, MAN_INF, "INF | main |   allow_rlf_llc: #", (int) llc_tx_config.allow_rlf);
        Logless(*main_logger, MAN_INF, "INF | main |   allow_rlf_pdcp: #", (int) pdcp_config.allow_rlf);
        Logless(*main_logger, MAN_INF, "INF | main |   allow_segmentation: #", (int) pdcp_config.allow_segmentation);
        Logless(*main_logger, MAN_INF, "INF | main |   allow_reordering: #", (int) pdcp_config.allow_reordering);
        if (pdcp_config.allow_rlf)
        Logless(*main_logger, MAN_INF, "INF | main |   max_sn_distance: #", pdcp_config.max_sn_distance);
        Logless(*main_logger, MAN_INF, "INF | main |   min_commit_size: #", pdcp_config.min_commit_size);
        Logless(*main_logger, MAN_INF, "INF | main |   type: #", ep_config.type.c_str());
        if (ep_config.type == "UDP")
        {
            ep_config.address1 = endpoint_config.at("udp_tx_address");
            ep_config.address2 = endpoint_config.at("udp_rx_address");
        Logless(*main_logger, MAN_INF, "INF | main |   udp_tx_address: #", ep_config.address1.c_str());
        Logless(*main_logger, MAN_INF, "INF | main |   udp_rx_address: #", ep_config.address2.c_str());
        }

        Logless(*main_logger, MAN_INF, "INF | main |");
    }

    if (app.count("txrx_device"))
    {
        rrc_config.app_config.wifi_device = app.at("txrx_device");
    }
    else if (app.count("wifi_over_udp"))
    {
        auto& wifi_over_udp = app.at("wifi_over_udp");
        rrc_config.app_config.woudp_tx = wifi_over_udp.at("tx_address");
        rrc_config.app_config.woudp_rx = wifi_over_udp.at("rx_address");
    }
    else
    {
        rrc_config.app_config.wifi_device = app.at("tx_device");
        rrc_config.app_config.wifi_device2 = app.at("rx_device");
    }

    rrc_config.app_config.udp_console = app.at("udp_console");

    Logless(*main_logger, MAN_INF, "INF | main | --- APP ---");
    Logless(*main_logger, MAN_INF, "INF | main | wifi_device1: #", rrc_config.app_config.wifi_device.c_str());
    Logless(*main_logger, MAN_INF, "INF | main | wifi_device2: #", rrc_config.app_config.wifi_device2.c_str());
    Logless(*main_logger, MAN_INF, "INF | main | woudp_tx: #", rrc_config.app_config.woudp_tx.c_str());
    Logless(*main_logger, MAN_INF, "INF | main | woudp_rx: #", rrc_config.app_config.woudp_rx.c_str());
    Logless(*main_logger, MAN_INF, "INF | main | udp_console #", rrc_config.app_config.udp_console.c_str());

    AppRRC app_rrc{rrc_config};
    app_rrc.run();
    return 0;
}