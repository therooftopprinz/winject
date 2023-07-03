#ifndef __APP_LOGGER_HPP__
#define __APP_LOGGER_HPP__

#include <logless/Logger.hpp>
#include <memory>

enum app_logbit_e {
              //     S D
    RRC_ERR,  //  0  1 1 1  
    RRC_WRN,  //  1  1 1 1  
    RRC_INF,  //  2  1 1 1  
    RRC_DBG,  //  3  1 1 1  
    RRC_TRC,  //  4  0 1 1  
    WTX_BUF,  //  5  0 1 0  WIFI TX buffer
    WTX_RAD,  //  6  0 1 0  WIFI TX radiotap
    WTX_802,  //  7  0 1 0  WIFI TX 802.11
    WRX_BUF,  //  8  0 1 0  WIFI RX buffer
    WRX_RAD,  //  9  0 1 0  WIFI RX radiotap
    WRX_802,  // 10  0 1 0  WIFI RX 802.11
    PDCP_ERR, // 11  1 1 1 
    PDCP_WRN, // 12  1 1 1 
    PDCP_INF, // 13  1 1 1 
    PDCP_DBG, // 14  1 1 1 
    PDCP_TRC, // 15  0 1 1 
    PDCP_STS, // 16  0 0 0  STATS
    LLC_ERR,  // 17  1 1 1 
    LLC_WRN,  // 18  1 1 1 
    LLC_INF,  // 19  1 1 1 
    LLC_DBG,  // 20  1 1 1 
    LLC_TRC,  // 21  0 1 1 
    LLC_STS,  // 22  0 0 0  STATS
    MAN_ERR,  // 23  1 1 1 
    MAN_WRN,  // 24  1 1 1 
    MAN_INF,  // 25  1 1 1 
    MAN_DBG,  // 26  1 1 1 
    MAN_TRC,  // 27  0 1 1 
    TXS_ERR,  // 28  1 1 1 
    TXS_WRN,  // 29  1 1 1 
    TXS_INF,  // 30  1 1 1 
    TXS_DBG,  // 31  1 1 1 
    TXS_TRC,  // 32  0 1 1 
    TEP_ERR,  // 33  1 1 1 
    TEP_WRN,  // 34  1 1 1 
    TEP_INF,  // 35  1 1 1 
    TEP_DBG,  // 36  1 1 1 
    TEP_TRC,  // 37  0 1 1
    MAX
};

using LoggerType = Logger<unsigned(app_logbit_e::MAX)>;
extern std::unique_ptr<LoggerType> main_logger;

#endif //__APP_LOGGER_HPP__
