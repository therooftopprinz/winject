#include <gtest/gtest.h>

#include "frame_defs.hpp"

//    +---+---+-----------------------+
// 00 | D | R | LLC SN                |
//    +---+---+-------+---+-----------+
// 01 | LCID          | C | SIZEH     |
//    +---------------+---+-----------+
// 02 | SIZEL                         |
//    +-------------------------------+
// 03 | Payload                       |
//    +-------------------------------+

TEST(Test_llc_t, should_set_ADR)
{
    uint8_t buffer[1024];
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_D(true);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(true, decode.get_D());
    }
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_D(false);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(false, decode.get_D());
    }
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_R(true);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(true, decode.get_R());
    }
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_R(false);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(false, decode.get_R());
    }
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_A(true);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(true, decode.get_A());
    }
    {
        llc_t encode(buffer, sizeof(buffer));
        encode.set_A(false);
        llc_t decode(buffer, sizeof(buffer));
        EXPECT_EQ(false, decode.get_A());
    }
}

TEST(Test_llc_t, should_set_LCID)
{
    uint8_t buffer[1024];
    llc_t encode(buffer, sizeof(buffer));
    encode.set_LCID(0xA);
    llc_t decode(buffer, sizeof(buffer));
    EXPECT_EQ(0xA, decode.get_LCID());
}

TEST(Test_llc_t, should_set_SIZE)
{
    uint8_t buffer[1024];
    llc_t encode(buffer, sizeof(buffer));
    encode.set_SIZE(0x5FA);
    llc_t decode(buffer, sizeof(buffer));
    EXPECT_EQ(0x5FA, decode.get_SIZE());
    EXPECT_EQ(0x5FA-3, decode.get_payload_size());
    EXPECT_EQ(sizeof(buffer)-3, encode.get_max_payload_size());
}