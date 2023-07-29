#include <gtest/gtest.h>

#include <arpa/inet.h>

#include "crc.hpp"

TEST(Test_crc, should_compute_crc32_0x04C11DB7)
{
    uint32_t res;

    const char* data = "hello world!";
    size_t data_len = strlen(data);
    res = crc<0x04C11DB7,32,uint32_t,true,true,0xFAFAFAFA,0xFCFCFCFC>()(data, data_len);
    EXPECT_EQ(res, 0xC4C04703);
}
