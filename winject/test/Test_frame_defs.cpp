/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <gtest/gtest.h>
#include <cstring>

#include "frame_defs.hpp"
#include "info_defs.hpp"

// =============================================================================
// basic_fec_t
// =============================================================================

TEST(Test_frame_defs_basic_fec_t, pointer_generation)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // E_FEC_TYPE_RS_255_247, n=1: fec_type, n, 8 bytes redundancy, 247 bytes data
    buffer[0] = static_cast<uint8_t>(E_FEC_TYPE_RS_255_247);
    buffer[1] = 1;
    {
        fec_t f(buffer, sizeof(buffer));
        f.rescan();
        ASSERT_TRUE(f.is_valid());
        EXPECT_EQ(buffer, f.fec_type);
        EXPECT_EQ(buffer + 1, f.n);
        EXPECT_EQ(buffer + 2, f.redu_blocks);
        EXPECT_EQ(buffer + 2 + 8, f.data_blocks);
    }
}

TEST(Test_frame_defs_basic_fec_t, setter_getter_vs_raw_bytes)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    buffer[0] = static_cast<uint8_t>(E_FEC_TYPE_RS_255_247);
    buffer[1] = 1;

    fec_t f(buffer, sizeof(buffer));
    f.rescan();
    ASSERT_TRUE(f.is_valid());

    // Set via pointers and compare to raw bytes
    *f.fec_type = static_cast<uint8_t>(E_FEC_TYPE_RS_255_239);
    *f.n = 2;
    EXPECT_EQ(buffer[0], static_cast<uint8_t>(E_FEC_TYPE_RS_255_239));
    EXPECT_EQ(buffer[1], 2);

    // Write into data_blocks and read back from raw buffer
    const size_t data_offset = 2 + 2 * (255 - 239);  // after redu_blocks for n=2, RS_255_239
    f.redu_blocks[0] = 0xAB;
    f.redu_blocks[1] = 0xCD;
    EXPECT_EQ(buffer[2], 0xAB);
    EXPECT_EQ(buffer[3], 0xCD);

    // Rescan and verify get (via re-reading)
    fec_t f2(buffer, sizeof(buffer));
    f2.rescan();
    ASSERT_TRUE(f2.is_valid());
    EXPECT_EQ(static_cast<uint8_t>(E_FEC_TYPE_RS_255_239), *f2.fec_type);
    EXPECT_EQ(2, *f2.n);
}

TEST(Test_frame_defs_basic_fec_t, nok_buffer_too_small)
{
    // Buffer too small for RS_255_247 with n=1 (needs 2 + 8 + 247 = 257 bytes)
    uint8_t small_buffer[10];
    small_buffer[0] = static_cast<uint8_t>(E_FEC_TYPE_RS_255_247);
    small_buffer[1] = 1;
    fec_t f(small_buffer, sizeof(small_buffer));
    f.rescan();
    EXPECT_FALSE(f.is_valid());
    EXPECT_EQ(nullptr, f.fec_type);
    EXPECT_EQ(nullptr, f.n);
    EXPECT_EQ(nullptr, f.data_blocks);
    EXPECT_EQ(nullptr, f.redu_blocks);
}

TEST(Test_frame_defs_basic_fec_t, nok_rescan_truncated)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    buffer[0] = static_cast<uint8_t>(E_FEC_TYPE_RS_255_247);
    buffer[1] = 1;

    // View only first 5 bytes – not enough for redu_blocks + data_blocks
    fec_t f(buffer, 5);
    f.rescan();
    EXPECT_FALSE(f.is_valid());
}

// =============================================================================
// basic_llc_t
// =============================================================================

TEST(Test_frame_defs_basic_llc_t, pointer_generation)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    llc_t llc(buffer, sizeof(buffer));
    // Header: SN(0) + LCID|A|SIZEH(1) + SIZEL(2) = 3 bytes; CRC follows if crc_size set
    llc.crc_size = 0;
    EXPECT_EQ(buffer, llc.base);
    EXPECT_EQ(buffer + 3, llc.payload());
    EXPECT_EQ(buffer + 3, llc.CRC());  // max_size non-zero so base+3
    llc.crc_size = 4;
    EXPECT_EQ(buffer + 3, llc.CRC());
    EXPECT_EQ(buffer + 3 + 4, llc.payload());
}

TEST(Test_frame_defs_basic_llc_t, setter_getter_vs_raw_bytes)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    llc_t llc(buffer, sizeof(buffer));
    llc.crc_size = 0;

    llc.set_SN(0xAB);
    llc.set_LCID(0x0F);
    llc.set_A(true);
    llc.set_SIZE(0x123);  // SIZEH=0x01, SIZEL=0x23

    EXPECT_EQ(buffer[0], 0xAB);
    EXPECT_EQ((buffer[1] & 0xF0) >> 4, 0x0F);
    EXPECT_TRUE((buffer[1] & 0x08) != 0);
    EXPECT_EQ(buffer[1] & 0x07, 0x01);
    EXPECT_EQ(buffer[2], 0x23);

    llc_const_t read(buffer, sizeof(buffer));
    read.crc_size = 0;
    EXPECT_EQ(read.get_SN(), 0xAB);
    EXPECT_EQ(read.get_LCID(), 0x0F);
    EXPECT_TRUE(read.get_A());
    EXPECT_EQ(read.get_SIZE(), 0x123);
}

TEST(Test_frame_defs_basic_llc_t, nok_size_overflow)
{
    uint8_t buffer[32];
    std::memset(buffer, 0, sizeof(buffer));
    llc_t llc(buffer, sizeof(buffer));
    llc.crc_size = 0;
    // is_valid() returns true when get_SIZE() > max_size (invalid/corrupt)
    llc.set_SIZE(1000);  // larger than 32
    llc_const_t read(buffer, sizeof(buffer));
    read.crc_size = 0;
    EXPECT_TRUE(read.is_valid());  // indicates invalid/corrupt frame
}

// =============================================================================
// basic_pdcp_segment_t
// =============================================================================

TEST(Test_frame_defs_basic_pdcp_segment_t, pointer_generation)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    // Minimal header: size only (2 bytes)
    {
        pdcp_segment_t seg(buffer, sizeof(buffer));
        seg.has_sn = false;
        seg.has_offset = false;
        seg.rescan();
        ASSERT_NE(nullptr, seg.size);
        EXPECT_EQ(buffer, reinterpret_cast<const uint8_t*>(seg.size));
        EXPECT_EQ(nullptr, seg.sn);
        EXPECT_EQ(nullptr, seg.offset);
        EXPECT_EQ(buffer + 2, seg.payload);
    }

    // With SN
    {
        pdcp_segment_t seg(buffer, sizeof(buffer));
        seg.has_sn = true;
        seg.has_offset = false;
        seg.rescan();
        EXPECT_EQ(buffer, reinterpret_cast<const uint8_t*>(seg.size));
        EXPECT_EQ(buffer + 2, reinterpret_cast<const uint8_t*>(seg.sn));
        EXPECT_EQ(nullptr, seg.offset);
        EXPECT_EQ(buffer + 4, seg.payload);
    }

    // With SN and offset
    {
        pdcp_segment_t seg(buffer, sizeof(buffer));
        seg.has_sn = true;
        seg.has_offset = true;
        seg.rescan();
        EXPECT_EQ(buffer, reinterpret_cast<const uint8_t*>(seg.size));
        EXPECT_EQ(buffer + 2, reinterpret_cast<const uint8_t*>(seg.sn));
        EXPECT_EQ(buffer + 4, reinterpret_cast<const uint8_t*>(seg.offset));
        EXPECT_EQ(buffer + 6, seg.payload);
    }
}

TEST(Test_frame_defs_basic_pdcp_segment_t, setter_getter_vs_raw_bytes)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    pdcp_segment_t seg(buffer, sizeof(buffer));
    seg.has_sn = true;
    seg.has_offset = true;
    seg.rescan();
    seg.set_SIZE(6 + 10);  // header 6 + payload 10
    seg.set_SN(0x1234);
    seg.set_OFFSET(0x5678);
    seg.set_LAST(true);

    // BEU16UA: big-endian. SIZE with L=1: 0x8000 | 16 = 0x8010
    EXPECT_EQ(buffer[0], 0x80);
    EXPECT_EQ(buffer[1], 0x10);
    // SN 0x1234
    EXPECT_EQ(buffer[2], 0x12);
    EXPECT_EQ(buffer[3], 0x34);
    // OFFSET 0x5678
    EXPECT_EQ(buffer[4], 0x56);
    EXPECT_EQ(buffer[5], 0x78);

    pdcp_segment_const_t read(buffer, sizeof(buffer));
    read.has_sn = true;
    read.has_offset = true;
    read.rescan();
    EXPECT_EQ(read.get_SIZE(), 16);
    EXPECT_TRUE(read.is_LAST());
    EXPECT_EQ(read.get_SN(), 0x1234);
    EXPECT_EQ(read.get_OFFSET(), 0x5678);
    EXPECT_EQ(read.get_payload_size(), 10);
    EXPECT_EQ(read.get_header_size(), 6u);
}

TEST(Test_frame_defs_basic_pdcp_segment_t, nok_buffer_too_small)
{
    uint8_t buffer[2];
    buffer[0] = 0x80;
    buffer[1] = 0xFF;  // size 0x7FFF
    pdcp_segment_t seg(buffer, 2);
    seg.has_sn = false;
    seg.has_offset = false;
    seg.rescan();
    // Header valid (2 bytes) but get_SIZE() is 0x7FFF, payload would extend past buffer
    EXPECT_FALSE(seg.is_valid());
}

TEST(Test_frame_defs_basic_pdcp_segment_t, nok_get_SN_without_has_sn)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    pdcp_segment_t seg(buffer, sizeof(buffer));
    seg.has_sn = false;
    seg.has_offset = false;
    seg.rescan();
    seg.set_SIZE(2);
    EXPECT_FALSE(seg.get_SN().has_value());
}

TEST(Test_frame_defs_basic_pdcp_segment_t, nok_get_OFFSET_without_has_offset)
{
    uint8_t buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    pdcp_segment_t seg(buffer, sizeof(buffer));
    seg.has_sn = false;
    seg.has_offset = false;
    seg.rescan();
    seg.set_SIZE(2);
    EXPECT_FALSE(seg.get_OFFSET().has_value());
}
