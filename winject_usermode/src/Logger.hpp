#ifndef __APP_LOGGER_HPP__
#define __APP_LOGGER_HPP__

#include <logless/Logger.hpp>
#include <memory>

enum app_logbit_e {
    RRC_ERR,  //  0 
    RRC_WRN,  //  1 
    RRC_INF,  //  2 
    RRC_DBG,  //  3 
    RRC_TRC,  //  4 
    WTX_BUF,  //  5 WIFI TX buffer
    WTX_RAD,  //  6 WIFI TX radiotap
    WTX_802,  //  7 WIFI TX 802.11
    WRX_BUF,  //  8 WIFI RX buffer
    WRX_RAD,  //  9 WIFI RX radiotap
    WRX_802,  // 10 WIFI RX 802.11
    PDCP_ERR, // 11
    PDCP_WRN, // 12
    PDCP_INF, // 13
    PDCP_DBG, // 14
    PDCP_TRC, // 15
    PDCP_STS, // 16 STATS
    LLC_ERR,  // 17
    LLC_WRN,  // 18
    LLC_INF,  // 19
    LLC_DBG,  // 20
    LLC_TRC,  // 21
    LLC_STS,  // 22 STATS
    MAN_ERR,  // 23
    MAN_WRN,  // 24
    MAN_INF,  // 25
    MAN_DBG,  // 26
    MAN_TRC,  // 27
    TXS_ERR,  // 28
    TXS_WRN,  // 29
    TXS_INF,  // 30
    TXS_DBG,  // 31
    TXS_TRC,  // 32
    TEP_ERR,  // 33
    TEP_WRN,  // 34
    TEP_INF,  // 35
    TEP_DBG,  // 36
    TEP_TRC,  // 37
    MAX
};

using LoggerType = Logger<unsigned(app_logbit_e::MAX)>;
extern std::unique_ptr<LoggerType> main_logger;

#endif //__APP_LOGGER_HPP__
