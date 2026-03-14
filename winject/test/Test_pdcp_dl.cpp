#include <gtest/gtest.h>

#include <pdcp/pdcp_dl.hpp>
#include <pdcp/pdcp_ul.hpp>
#include <bfc/buffer.hpp>
#include <cstring>
#include <vector>

using namespace winject;

struct Test_pdcp_dl : public ::testing::Test
{
    pdcp_dl_config_t make_dl_config(bool allow_segmentation,
                                    bool allow_reordering) const
    {
        pdcp_dl_config_t cfg{};
        cfg.allow_segmentation = allow_segmentation;
        cfg.allow_reordering   = allow_reordering;
        return cfg;
    }

    pdcp_ul_config_t make_ul_config(bool allow_segmentation,
                                    bool allow_reordering) const
    {
        pdcp_ul_config_t cfg{};
        cfg.allow_segmentation = allow_segmentation;
        cfg.allow_reordering   = allow_reordering;
        cfg.min_commit_size    = 4;
        return cfg;
    }
};

TEST_F(Test_pdcp_dl, should_not_accept_empty_pdcp_buffer)
{
    pdcp_dl sut(make_dl_config(false, false));

    bfc::buffer_view empty_view{};
    EXPECT_FALSE(sut.on_pdcp_data(empty_view));
    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());
    EXPECT_EQ(pdcp_dl::STATUS_CODE_INPUT_EMPTY, sut.get_status());
}

TEST_F(Test_pdcp_dl, should_reassemble_single_unsegmented_frame)
{
    // Use UL to generate a single PDCP segment and DL to reassemble it.
    constexpr size_t payload_size = 10;
    std::byte        in_data[payload_size];
    for (size_t i = 0; i < payload_size; ++i)
    {
        in_data[i] = static_cast<std::byte>(0x30 + i);
    }

    pdcp_ul ul(make_ul_config(false, true)); // single segment with SN/OFFSET

    bfc::buffer_view in_view{in_data, payload_size};
    ASSERT_TRUE(ul.on_frame_data(in_view));

    uint8_t          pdcp_storage[128]{};
    bfc::buffer_view pdcp_view{pdcp_storage, sizeof(pdcp_storage)};

    auto written = ul.write_pdcp(pdcp_view);
    ASSERT_GT(written, 0);

    pdcp_dl sut(make_dl_config(false, true)); // parse SN/OFFSET

    bfc::buffer_view pdcp_in{pdcp_storage, static_cast<size_t>(written)};
    ASSERT_TRUE(sut.on_pdcp_data(pdcp_in));

    EXPECT_EQ(1u, sut.get_outstanding_packet());
    EXPECT_EQ(payload_size, sut.get_outstanding_bytes());

    bfc::sized_buffer out = sut.pop();
    ASSERT_EQ(payload_size, out.size());
    EXPECT_EQ(0,
              std::memcmp(out.data(),
                          in_data,
                          payload_size));
    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());
}

TEST_F(Test_pdcp_dl, should_reassemble_segmented_frame)
{
    pdcp_ul ul(make_ul_config(true, true)); // segmentation + reordering

    constexpr size_t total_payload = 10;
    std::byte        in_data[total_payload];
    for (size_t i = 0; i < total_payload; ++i)
    {
        in_data[i] = static_cast<std::byte>(i + 1);
    }

    bfc::buffer_view in_view{in_data, total_payload};
    ASSERT_TRUE(ul.on_frame_data(in_view));

    // First call: small output buffer forces segmentation.
    uint8_t          out_first[8]{};
    bfc::buffer_view out_view_first{out_first, sizeof(out_first)};
    auto             written_first = ul.write_pdcp(out_view_first);

    ASSERT_GT(written_first, 0);

    // Second call: should emit remaining bytes for the same frame.
    uint8_t          out_second[32]{};
    bfc::buffer_view out_view_second{out_second, sizeof(out_second)};
    auto             written_second = ul.write_pdcp(out_view_second);

    ASSERT_GT(written_second, 0);

    pdcp_dl sut(make_dl_config(true, true)); // segmentation + reordering

    // Feed first segment; should not yet have a complete frame.
    bfc::buffer_view seg1_view{out_first,
                               static_cast<size_t>(written_first)};
    ASSERT_TRUE(sut.on_pdcp_data(seg1_view));
    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());

    // Feed second segment; frame should now be complete.
    bfc::buffer_view seg2_view{out_second,
                               static_cast<size_t>(written_second)};
    ASSERT_TRUE(sut.on_pdcp_data(seg2_view));

    EXPECT_EQ(1u, sut.get_outstanding_packet());
    EXPECT_EQ(total_payload, sut.get_outstanding_bytes());

    bfc::sized_buffer out = sut.pop();
    ASSERT_EQ(total_payload, out.size());
    EXPECT_EQ(0,
              std::memcmp(out.data(),
                          in_data,
                          total_payload));
}

TEST_F(Test_pdcp_dl, should_flush_completed_frames_in_order)
{
    // Prepare two frames on UL side.
    pdcp_ul ul(make_ul_config(false, true)); // no segmentation, with SN/OFFSET

    constexpr size_t payload_size = 5;
    std::byte        frame0[payload_size];
    std::byte        frame1[payload_size];

    for (size_t i = 0; i < payload_size; ++i)
    {
        frame0[i] = static_cast<std::byte>(0x10 + i);
        frame1[i] = static_cast<std::byte>(0x20 + i);
    }

    bfc::buffer_view frame0_view{frame0, payload_size};
    bfc::buffer_view frame1_view{frame1, payload_size};
    ASSERT_TRUE(ul.on_frame_data(frame0_view));
    ASSERT_TRUE(ul.on_frame_data(frame1_view));

    // Write both frames into a single PDCP buffer.
    uint8_t          pdcp_storage[128]{};
    bfc::buffer_view pdcp_view{pdcp_storage, sizeof(pdcp_storage)};
    auto             written = ul.write_pdcp(pdcp_view);
    ASSERT_GT(written, 0);

    // Split the buffer back into individual segments, tracking SN.
    struct segment_t
    {
        std::vector<uint8_t> bytes;
        pdcp_sn_t            sn;
    };

    std::vector<segment_t> segments;
    size_t                 remaining = static_cast<size_t>(written);
    uint8_t*               cursor    = pdcp_storage;

    while (remaining)
    {
        pdcp_segment_t seg(cursor, remaining, true, false);
        seg.rescan();
        ASSERT_TRUE(seg.is_valid());
        size_t seg_size = static_cast<size_t>(seg.get_SIZE());
        ASSERT_LE(seg_size, remaining);

        segment_t s{};
        s.bytes.assign(cursor, cursor + seg_size);
        s.sn = seg.get_SN();
        segments.emplace_back(std::move(s));

        cursor    += seg_size;
        remaining -= seg_size;
    }

    ASSERT_EQ(2u, segments.size());
    ASSERT_NE(segments[0].sn, segments[1].sn);

    // Identify which segment has SN 0 and which has SN 1.
    const segment_t* seg_sn0 = nullptr;
    const segment_t* seg_sn1 = nullptr;
    for (const auto& s : segments)
    {
        if (s.sn == 0)
        {
            seg_sn0 = &s;
        }
        else if (s.sn == 1)
        {
            seg_sn1 = &s;
        }
    }

    ASSERT_NE(nullptr, seg_sn0);
    ASSERT_NE(nullptr, seg_sn1);

    pdcp_dl sut(make_dl_config(false, true)); // parse SN/OFFSET and reorder

    // First, deliver SN 1: this should not yet be popped.
    {
        auto*               data_sn1 = const_cast<uint8_t*>(seg_sn1->bytes.data());
        bfc::buffer_view    view_sn1{data_sn1,
                                     seg_sn1->bytes.size()};
        ASSERT_TRUE(sut.on_pdcp_data(view_sn1));
        EXPECT_EQ(0u, sut.get_outstanding_packet());
        EXPECT_EQ(0u, sut.get_outstanding_bytes());
    }

    // Then deliver SN 0: flush_completed_in_order must now emit both frames
    // in SN order (0, then 1).
    {
        auto*               data_sn0 = const_cast<uint8_t*>(seg_sn0->bytes.data());
        bfc::buffer_view    view_sn0{data_sn0,
                                     seg_sn0->bytes.size()};
        ASSERT_TRUE(sut.on_pdcp_data(view_sn0));
    }

    EXPECT_EQ(2u, sut.get_outstanding_packet());
    EXPECT_EQ(payload_size * 2, sut.get_outstanding_bytes());

    bfc::sized_buffer out0 = sut.pop();
    ASSERT_EQ(payload_size, out0.size());
    EXPECT_EQ(0,
              std::memcmp(out0.data(),
                          frame0,
                          payload_size));

    bfc::sized_buffer out1 = sut.pop();
    ASSERT_EQ(payload_size, out1.size());
    EXPECT_EQ(0,
              std::memcmp(out1.data(),
                          frame1,
                          payload_size));

    EXPECT_EQ(0u, sut.get_outstanding_packet());
    EXPECT_EQ(0u, sut.get_outstanding_bytes());
}
