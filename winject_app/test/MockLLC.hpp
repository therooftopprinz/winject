#ifndef __WINJECTUMTST_MOCKLLC_HPP__
#define __WINJECTUMTST_MOCKLLC_HPP__

#include <gmock/gmock.h>
#include "ILLC.hpp"

struct MockLLC : public ILLC
{
    MOCK_METHOD1(on_tx, void(tx_info_t&));
    MOCK_METHOD1(on_rx, void(rx_info_t&));
    MOCK_METHOD1(set_tx_enabled, void(bool));
    MOCK_METHOD1(set_rx_enabled, void(bool));
    MOCK_METHOD0(get_tx_confg, tx_config_t());
    MOCK_METHOD0(get_rx_confg, rx_config_t());
    MOCK_METHOD1(reconfigure, void(const tx_config_t&));
    MOCK_METHOD1(reconfigure, void(const rx_config_t&));
    MOCK_METHOD0(get_lcid, lcid_t());
    MOCK_METHOD0(get_stats, const stats_t&());
};

#endif // __WINJECTUMTST_MOCKLLC_HPP__