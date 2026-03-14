#include <gtest/gtest.h>

#include <pdcp/pdcp_ul.hpp>
#include <bfc/buffer.hpp>
#include <cstring>

using namespace winject;

struct Test_pdcp_ul : public ::testing::Test
{
    pdcp_ul_config_t make_config(bool allow_segmentation,
                                 bool allow_reordering) const
    {
        pdcp_ul_config_t cfg{};
        cfg.allow_segmentation = allow_segmentation;
        cfg.allow_reordering   = allow_reordering;
        cfg.min_commit_size    = 4;
        return cfg;
    }
};

TEST_F(Test_pdcp_ul, should_not_accept_empty_packet)
{
    pdcp_ul sut(make_config(false, false));

    bfc::buffer_view empty_view{};
    EXPECT_FALSE(sut.on_frame_data(empty_view));
    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());
}

TEST_F(Test_pdcp_ul, should_buffer_single_frame)
{
    pdcp_ul sut(make_config(false, false));

    std::byte data[5];
    for (size_t i = 0; i < sizeof(data); ++i)
    {
        data[i] = static_cast<std::byte>(0x10 + i);
    }

    bfc::buffer_view view{data, sizeof(data)};
    ASSERT_TRUE(sut.on_frame_data(view));

    EXPECT_EQ(1u, sut.get_outstanding_packet());
    EXPECT_EQ(sizeof(data), sut.get_outstanding_bytes());
}

TEST_F(Test_pdcp_ul, should_fail_write_when_no_data)
{
    pdcp_ul sut(make_config(false, true));

    std::byte out_storage[16]{};
    bfc::buffer_view out_view{out_storage, sizeof(out_storage)};

    auto written = sut.write_pdcp(out_view);
    EXPECT_EQ(-1, written);
    EXPECT_EQ(pdcp_ul::STATUS_CODE_NO_DATA, sut.get_status());
}

TEST_F(Test_pdcp_ul, should_set_buffer_too_small_on_empty_output)
{
    pdcp_ul sut(make_config(false, true));

    std::byte in_data[4];
    bfc::buffer_view in_view{in_data, sizeof(in_data)};
    ASSERT_TRUE(sut.on_frame_data(in_view));

    bfc::buffer_view empty_out{};
    auto written = sut.write_pdcp(empty_out);

    EXPECT_EQ(-1, written);
    EXPECT_EQ(pdcp_ul::STATUS_CODE_OUTPUT_EMPTY, sut.get_status());
    EXPECT_EQ(1u, sut.get_outstanding_packet());
}

TEST_F(Test_pdcp_ul, should_write_single_segment_without_segmentation)
{
    pdcp_ul sut(make_config(false, true)); // no segmentation, with SN/OFFSET

    constexpr size_t payload_size = 5;
    std::byte in_data[payload_size];
    for (size_t i = 0; i < payload_size; ++i)
    {
        in_data[i] = static_cast<std::byte>(0x20 + i);
    }

    bfc::buffer_view in_view{in_data, payload_size};
    ASSERT_TRUE(sut.on_frame_data(in_view));

    uint8_t out_storage[64]{};
    bfc::buffer_view out_view{out_storage, sizeof(out_storage)};

    auto written = sut.write_pdcp(out_view);
    ASSERT_GT(written, 0);
    EXPECT_EQ(pdcp_ul::STATUS_CODE_SUCCESS, sut.get_status());
    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());

    pdcp_segment_t segment(out_storage, static_cast<size_t>(written), true, false);
    segment.rescan();
    ASSERT_TRUE(segment.is_valid());

    EXPECT_EQ(0, segment.get_SN());
    EXPECT_TRUE(segment.is_LAST());
    EXPECT_EQ(payload_size, static_cast<size_t>(segment.get_payload_size()));

    // `written` must be the full PDCP size (header + payload).
    EXPECT_EQ(static_cast<size_t>(written),
              static_cast<size_t>(segment.get_SIZE()));

    EXPECT_EQ(0, std::memcmp(segment.get_payload(),
                             in_data,
                             payload_size));
}

TEST_F(Test_pdcp_ul, should_segment_frame_when_segmentation_enabled)
{
    pdcp_ul sut(make_config(true, true)); // segmentation + reordering

    constexpr size_t total_payload = 10;
    std::byte in_data[total_payload];
    for (size_t i = 0; i < total_payload; ++i)
    {
        in_data[i] = static_cast<std::byte>(i);
    }

    bfc::buffer_view in_view{in_data, total_payload};
    ASSERT_TRUE(sut.on_frame_data(in_view));

    // First call: small output buffer forces segmentation
    uint8_t out_first[8]{};
    bfc::buffer_view out_view_first{out_first, sizeof(out_first)};
    auto written_first = sut.write_pdcp(out_view_first);

    ASSERT_GT(written_first, 0);
    EXPECT_EQ(pdcp_ul::STATUS_CODE_SUCCESS, sut.get_status());
    EXPECT_GT(sut.get_outstanding_bytes(), 0u);

    pdcp_segment_t first_seg(out_first, static_cast<size_t>(written_first), true, true);
    first_seg.rescan();
    ASSERT_TRUE(first_seg.is_valid());

    auto first_payload_size = first_seg.get_payload_size();
    EXPECT_FALSE(first_seg.is_LAST());
    ASSERT_GT(first_payload_size, 0);
    ASSERT_LT(static_cast<size_t>(first_payload_size), total_payload);

    EXPECT_EQ(0, first_seg.get_SN());
    EXPECT_EQ(0, first_seg.get_OFFSET());

    // First call must report the full PDCP size of the first segment.
    EXPECT_EQ(static_cast<size_t>(written_first),
              static_cast<size_t>(first_seg.get_SIZE()));
    EXPECT_EQ(0, std::memcmp(first_seg.get_payload(),
                             in_data,
                             static_cast<size_t>(first_payload_size)));

    // Second call: should emit the remaining bytes for the same frame
    uint8_t out_second[32]{};
    bfc::buffer_view out_view_second{out_second, sizeof(out_second)};
    auto written_second = sut.write_pdcp(out_view_second);

    ASSERT_GT(written_second, 0);
    EXPECT_EQ(pdcp_ul::STATUS_CODE_SUCCESS, sut.get_status());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());

    pdcp_segment_t second_seg(out_second, static_cast<size_t>(written_second), true, true);
    second_seg.rescan();
    ASSERT_TRUE(second_seg.is_valid());

    auto second_payload_size = second_seg.get_payload_size();
    EXPECT_TRUE(second_seg.is_LAST());
    EXPECT_EQ(0, second_seg.get_SN()); // same frame, same SN

    // Second call must report the full PDCP size of the second segment.
    EXPECT_EQ(static_cast<size_t>(written_second),
              static_cast<size_t>(second_seg.get_SIZE()));

    auto total_from_segments =
        static_cast<size_t>(first_payload_size) +
        static_cast<size_t>(second_payload_size);
    EXPECT_EQ(total_payload, total_from_segments);

    EXPECT_EQ(0,
              std::memcmp(first_seg.get_payload(),
                          in_data,
                          static_cast<size_t>(first_payload_size)));
    EXPECT_EQ(0,
              std::memcmp(second_seg.get_payload(),
                          in_data + first_payload_size,
                          static_cast<size_t>(second_payload_size)));
}

