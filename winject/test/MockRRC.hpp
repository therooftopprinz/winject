#ifndef __WINJECTUMTST_MOCKRRC_HPP__
#define __WINJECTUMTST_MOCKRRC_HPP__

#include <gmock/gmock.h>
#include "IRRC.hpp"

struct MockRRC : public IRRC
{
    MOCK_METHOD1(on_rlf, void(lcid_t));
    MOCK_METHOD1(on_init, void(lcid_t));
    MOCK_METHOD0(allocate_req_id, uint8_t());
    MOCK_METHOD1(send_rrc, void(const RRC& rrc));
    MOCK_METHOD1(perform_tx, void(size_t));
    MOCK_METHOD0(stop, void());
};

#endif // __WINJECTUMTST_MOCKRRC_HPP__