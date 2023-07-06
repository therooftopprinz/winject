#ifndef __WINJECTUMTST_MOCKPDCP_HPP__
#define __WINJECTUMTST_MOCKPDCP_HPP__

#include <gmock/gmock.h>
#include "IPDCP.hpp"

struct MockPDCP : public IPDCP
{
    MOCK_METHOD1(on_tx, void(tx_info_t&));
    MOCK_METHOD1(on_rx, void(rx_info_t&));
    MOCK_METHOD1(to_rx, buffer_t(uint64_t));
    MOCK_METHOD1(to_tx, bool(buffer_t));
    MOCK_METHOD0(get_attached_lcid, lcid_t ());
    MOCK_METHOD1(set_tx_enabled, void(bool));
    MOCK_METHOD1(set_rx_enabled, void(bool));
    MOCK_METHOD1(reconfigure, void(const rx_config_t& config));
    MOCK_METHOD1(reconfigure, void(const tx_config_t& config));
    MOCK_METHOD0(get_rx_config, rx_config_t());
    MOCK_METHOD0(get_tx_config, tx_config_t());
    MOCK_METHOD0(get_stats, const stats_t&());
    
};

#endif // __WINJECTUMTST_MOCKPDCP_HPP__