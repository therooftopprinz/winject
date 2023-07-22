#ifndef __APP_LOGGER_HPP__
#define __APP_LOGGER_HPP__

#include <logless/Logger.hpp>
#include <bfc/Metric.hpp>
#include <memory>

enum app_logbit_e {
              /****
               *  X - silent
               *  T - Tick
               *
               *
              */
              //    X D S T
    RRC_ERR,  //  0 0 1 1 1
    RRC_WRN,  //  1 0 1 1 1
    RRC_INF,  //  2 0 1 1 1
    RRC_DBG,  //  3 0 1 0 1
    RRC_TRC,  //  4 0 1 0 1
    WTX_BUF,  //  5 0 1 0 1 WIFI TX buffer
    WTX_RAD,  //  6 0 1 0 1 WIFI TX radiotap
    WTX_802,  //  7 0 1 0 1 WIFI TX 802.11
    WRX_BUF,  //  8 0 1 0 1 WIFI RX buffer
    WRX_RAD,  //  9 0 1 0 1 WIFI RX radiotap
    WRX_802,  // 10 0 1 0 1 WIFI RX 802.11
    PDCP_ERR, // 11 0 1 1 1
    PDCP_WRN, // 12 0 1 1 1
    PDCP_INF, // 13 0 1 1 1
    PDCP_DBG, // 14 0 1 0 1
    PDCP_TRC, // 15 0 1 0 1
    LLC_ERR,  // 16 0 1 1 1
    LLC_WRN,  // 17 0 1 1 1
    LLC_INF,  // 18 0 1 1 1
    LLC_DBG,  // 19 0 1 0 1
    LLC_TRC,  // 20 0 1 0 1
    MAN_ERR,  // 21 0 1 1 1
    MAN_WRN,  // 22 0 1 1 1
    MAN_INF,  // 23 0 1 1 1
    MAN_DBG,  // 24 0 1 1 1
    MAN_TRC,  // 25 0 1 1 1
    TXS_ERR,  // 26 0 1 1 1
    TXS_WRN,  // 27 0 1 1 1
    TXS_INF,  // 28 0 1 1 1
    TXS_DBG,  // 29 0 1 0 1
    TXS_TRC,  // 30 0 1 0 1
    TXS_TIC,  // 31 0 0 0 0 TICK
    TEP_ERR,  // 32 0 1 1 1
    TEP_WRN,  // 33 0 1 1 1
    TEP_INF,  // 34 0 1 1 1
    TEP_DBG,  // 35 0 1 0 1
    TEP_TRC,  // 36 0 1 0 1
    MAX
};

inline std::string to_llc_stat(std::string statname, size_t lcid)
{
    std::stringstream ss;
    ss << "llc_" << statname << "{"
       << "lcid=\"" << lcid << "\""
       <<"}";
    return ss.str();
}

inline std::string to_pdcp_stat(std::string statname, size_t lcid)
{
    std::stringstream ss;
    ss << "pdcp_" << statname << "{"
       << "lcid=\"" << lcid << "\""
       <<"}";
    return ss.str();
}

using LoggerType = Logger<unsigned(app_logbit_e::MAX)>;
extern std::unique_ptr<LoggerType> main_logger;
extern std::unique_ptr<bfc::IMonitor> main_monitor;

#endif //__APP_LOGGER_HPP__
