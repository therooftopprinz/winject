#ifndef __WINJECTUMTST_MOCKRRC_HPP__
#define __WINJECTUMTST_MOCKRRC_HPP__

#include <gmock/gmock.h>
#include "IRRC.hpp"

struct MockRRC : public IRRC
{
    MOCK_METHOD1(on_rlf, void(lcid_t));
    MOCK_METHOD1(perform_tx, void(size_t));
};

#endif // __WINJECTUMTST_MOCKRRC_HPP__