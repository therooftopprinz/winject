#ifndef __WINJECTUMTST_MOCKPDCP_HPP__
#define __WINJECTUMTST_MOCKPDCP_HPP__

#include <gmock/gmock.h>
#include "IPDCP.hpp"

struct MockPDCP : public IPDCP
{
    MOCK_METHOD1(on_tx, void(tx_info_t&));
    MOCK_METHOD1(on_rx, void(rx_info_t&));
    MOCK_METHOD0(on_rlf, void());
};

#endif // __WINJECTUMTST_MOCKPDCP_HPP__