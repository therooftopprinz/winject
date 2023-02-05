#include <gtest/gtest.h>

#include "frame_defs.hpp"

//    +-------------------------------+
// 00 | IV (OPTIONAL)                 | IVSZ
//    +-------------------------------+
//    | HMAC (OPTIONAL)               | HMSZ
//    +-------------------------------|
//    | PAYLOAD                       | EOP
//    +-------------------------------+

TEST(Test_pdcp_t, should_get_pointers)
{
    uint8_t buffer[1024];
    {
        pdcp_t encode(buffer, sizeof(buffer));
        encode.iv_size = 0;
        encode.hmac_size = 0;
        encode.rescan();
        EXPECT_EQ(nullptr, encode.iv);
        EXPECT_EQ(nullptr, encode.hmac);
        EXPECT_EQ(buffer, encode.payload);
    }
    {
        pdcp_t encode(buffer, sizeof(buffer));
        encode.iv_size = 0;
        encode.hmac_size = 2;
        encode.rescan();
        EXPECT_EQ(nullptr, encode.iv);
        EXPECT_EQ(buffer, encode.hmac);
        EXPECT_EQ(buffer+2, encode.payload);
    }
    {
        pdcp_t encode(buffer, sizeof(buffer));
        encode.iv_size = 4;
        encode.hmac_size = 4;
        encode.rescan();
        EXPECT_EQ(buffer, encode.iv);
        EXPECT_EQ(buffer+4, encode.hmac);
        EXPECT_EQ(buffer+8, encode.payload);
    }
}

//     +-------------------------------+
//  00 | PACKET SN                     |
//     +-------------------------------|
//  01 | OFFSET (OPTIONAL)             | 02
//     +-------------------------------+
//     | SIZE                          | 02
//     +-------------------------------+
//     | PAYLOAD                       | EOS
//     +-------------------------------+

TEST(Test_pdcp_segment_t, should_set_fields)
{
    uint8_t buffer[1024];

    {
        pdcp_segment_t encode(buffer, sizeof(buffer));
        encode.rescan();
        encode.set_SN(5);
        encode.set_payload_size(1);
        pdcp_segment_t decode(buffer, encode.get_SIZE());
        decode.rescan();
        EXPECT_EQ(5, decode.get_SN());
        EXPECT_EQ(1, decode.get_payload_size());
        EXPECT_EQ(buffer+decode.get_header_size(), decode.payload);
    }
    {
        pdcp_segment_t encode(buffer, sizeof(buffer));
        encode.has_offset = true;
        encode.rescan();
        encode.set_SN(8);
        encode.set_OFFSET(10);
        encode.set_payload_size(5);
        pdcp_segment_t decode(buffer, encode.get_SIZE());
        decode.has_offset = true;
        decode.rescan();
        EXPECT_EQ(8, decode.get_SN());
        EXPECT_EQ(10, decode.get_OFFSET());
        EXPECT_EQ(5, decode.get_payload_size());
        EXPECT_EQ(buffer+decode.get_header_size(), decode.payload);
    }
}