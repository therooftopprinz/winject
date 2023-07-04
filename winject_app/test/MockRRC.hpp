#ifndef __WINJECTUMTST_MOCKRRC_HPP__
#define __WINJECTUMTST_MOCKRRC_HPP__

#include <gmock/gmock.h>
#include "IRRC.hpp"

struct MockRRC : public IRRC
{
    MOCK_METHOD1(on_rlf_tx, void(lcid_t));
    MOCK_METHOD1(on_rlf_rx, void(lcid_t));
    MOCK_METHOD1(initialize_tx, void(lcid_t));
    MOCK_METHOD1(perform_tx, void(size_t));
};

#endif // __WINJECTUMTST_MOCKRRC_HPP__