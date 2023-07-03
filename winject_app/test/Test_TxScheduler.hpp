#include <gtest/gtest.h>
#include "TxScheduler.hpp"
#include "MockLLC.hpp"
#include "StubTimer.hpp"
#include "buffer_utils.hpp"

struct Test_TxScheduler : public testing::Test
{
    Test_TxScheduler()
    {
    }

    std::shared_ptr<TxScheduler> sut;
    uint8_t buffer[1024];
};

TEST_F(Test_TxScheduler, __)
{
}
