#ifndef __WINJECTUMTST_MOCKPDCP_HPP__
#define __WINJECTUMTST_MOCKPDCP_HPP__

#include <gmock/gmock.h>
#include "IPDCP.hpp"

struct MockPDCP : public IPDCP
{
    MOCK_METHOD1(on_tx, void(tx_info_t&));
    MOCK_METHOD1(on_rx, void(rx_info_t&));
};

#endif // __WINJECTUMTST_MOCKPDCP_HPP__