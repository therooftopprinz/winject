#include <gtest/gtest.h>
#include "PDCP.hpp"
#include "info_defs_matcher.hpp"
#include "buffer_utils.hpp"

struct Test_PDCP : public testing::Test
{
    Test_PDCP()
    {

    }

    void configure_sut_basic()
    {
        IPDCP::tx_config_t tx_config{};
        tx_config.min_commit_size = 5;
        tx_config.allow_segmentation = false;
        tx_config.allow_reordering = true;
        IPDCP::rx_config_t rx_config{};

        sut = std::make_shared<PDCP>(0, tx_config, rx_config);
        sut->set_tx_enabled(true);
        sut->set_rx_enabled(true);
    }

    void configure_sut_segmented()
    {
        IPDCP::tx_config_t tx_config{};
        tx_config.min_commit_size = 5;
        tx_config.allow_segmentation = true;
        tx_config.allow_reordering = true;
        IPDCP::rx_config_t rx_config{};

        sut = std::make_shared<PDCP>(0, tx_config, rx_config);
        sut->set_tx_enabled(true);
        sut->set_rx_enabled(true);
    }


    tx_info_t trigger_tx(size_t slot_number, uint8_t* buffer, size_t size)
    {
        tx_frame_info.slot_number = slot_number;
        tx_info_t tx_info{tx_frame_info};
        tx_info.out_tx_available = false;
        tx_info.out_pdu.base = buffer;
        tx_info.out_pdu.size = size;
        sut->on_tx(tx_info);
        return tx_info;
    }

    std::shared_ptr<PDCP> sut;
    frame_info_t tx_frame_info{};
    frame_info_t rx_frame_info{};
    uint8_t packet[1024];
    uint8_t buffer[1024];
    uint8_t cmp[1024];
};

TEST_F(Test_PDCP, should_not_allocate_pdcp_when_pdu_size_is_zero)
{
    configure_sut_basic();
    auto tx_info = trigger_tx(42, nullptr, 0);
    EXPECT_EQ(0, tx_info.out_allocated);
}

TEST_F(Test_PDCP, should_report_tx_available)
{
    configure_sut_basic();
    sut->to_tx(to_buffer("ABCD1122"));
    auto tx_info = trigger_tx(42, nullptr, 0);
    EXPECT_TRUE(tx_info.out_tx_available);
    EXPECT_EQ(0, tx_info.out_allocated);
}

TEST_F(Test_PDCP, should_allocate_basic)
{
    configure_sut_basic();
    sut->to_tx(to_buffer("ABCD112233"));
    auto tx_info = trigger_tx(42, buffer, sizeof(buffer));

    pdcp_t pdcp(buffer, sizeof(buffer));
    pdcp.iv_size = 0;
    pdcp.hmac_size = 0;
    pdcp.rescan();

    auto payload = pdcp.payload;
    pdcp_segment_t segment(payload, pdcp.pdu_size);
    segment.has_offset = false;
    segment.has_sn = true;
    segment.rescan();

    EXPECT_EQ(0, segment.get_SN());
    EXPECT_EQ(5, segment.get_payload_size());
    EXPECT_EQ(pdcp.get_header_size() + segment.get_SIZE(), tx_info.out_allocated);
    EXPECT_FALSE(tx_info.out_tx_available);
}

TEST_F(Test_PDCP, should_allocate_segmented_halving)
{
    configure_sut_segmented();
    tx_frame_info.frame_payload_size = 1000;
    sut->to_tx(to_buffer("ABCD112233EFAA445566"));
    {
        auto tx_info = trigger_tx(1, buffer, 10);

        ASSERT_TRUE(tx_info.out_allocated);

        pdcp_t pdcp(buffer, sizeof(buffer));
        pdcp.iv_size = 0;
        pdcp.hmac_size = 0;
        pdcp.rescan();

        auto payload = pdcp.payload;
        pdcp_segment_t segment(payload, 5);
        segment.has_offset = true;
        segment.has_sn = true;
        segment.rescan();

        EXPECT_EQ(0, segment.get_SN());
        EXPECT_EQ(pdcp.get_header_size() + segment.get_SIZE(), tx_info.out_allocated);
        EXPECT_TRUE(tx_info.out_tx_available);

        ASSERT_EQ(5, segment.get_payload_size());

        to_buffer_direct(cmp, "ABCD112233");
        EXPECT_EQ(0, std::memcmp(segment.payload, cmp, 5));
    }
    {
        auto tx_info = trigger_tx(2, buffer, 10);

        ASSERT_TRUE(tx_info.out_allocated);

        pdcp_t pdcp(buffer, sizeof(buffer));
        pdcp.iv_size = 0;
        pdcp.hmac_size = 0;
        pdcp.rescan();

        auto payload = pdcp.payload;
        pdcp_segment_t segment(payload, 5);
        segment.has_offset = true;
        segment.has_sn = true;
        segment.rescan();

        EXPECT_EQ(1, segment.get_SN());
        EXPECT_EQ(pdcp.get_header_size() + segment.get_SIZE(), tx_info.out_allocated);
        EXPECT_FALSE(tx_info.out_tx_available);

        ASSERT_EQ(5, segment.get_payload_size());

        to_buffer_direct(cmp, "EFAA445566");
        EXPECT_EQ(0, std::memcmp(segment.payload, cmp, 5));
    }
}


TEST_F(Test_PDCP, should_allocate_segmented_carry_over)
{
    configure_sut_segmented();
    tx_frame_info.frame_payload_size = 1000;
    sut->to_tx(to_buffer("ABCD112233EFAA445566"));
    sut->to_tx(to_buffer("001122"));
    {
        auto tx_info = trigger_tx(1, buffer, 10);

        ASSERT_TRUE(tx_info.out_allocated);

        pdcp_t pdcp(buffer, sizeof(buffer));
        pdcp.iv_size = 0;
        pdcp.hmac_size = 0;
        pdcp.rescan();

        auto payload = pdcp.payload;
        pdcp_segment_t segment(payload, 5);
        segment.has_offset = true;
        segment.has_sn = true;
        segment.rescan();

        EXPECT_EQ(0, segment.get_SN());
        EXPECT_EQ(pdcp.get_header_size() + segment.get_SIZE(), tx_info.out_allocated);
        EXPECT_TRUE(tx_info.out_tx_available);

        ASSERT_EQ(5, segment.get_payload_size());

        to_buffer_direct(cmp, "ABCD112233");
        EXPECT_EQ(0, std::memcmp(segment.payload, cmp, 5));
    }
    {
        auto tx_info = trigger_tx(2, buffer, 100);

        ASSERT_TRUE(tx_info.out_allocated);

        pdcp_t pdcp(buffer, sizeof(buffer));
        pdcp.iv_size = 0;
        pdcp.hmac_size = 0;
        pdcp.rescan();

        // SEGMENT 1
        auto payload = pdcp.payload;
        pdcp_segment_t segment(payload, 5);
        segment.has_offset = true;
        segment.has_sn = true;
        segment.rescan();

        EXPECT_EQ(1, segment.get_SN());
        ASSERT_EQ(5, segment.get_payload_size());

        to_buffer_direct(cmp, "EFAA445566");
        EXPECT_EQ(0, std::memcmp(segment.payload, cmp, 5));

        // SEGMENT 2
        segment.base += segment.get_SIZE();
        segment.has_offset = true;
        segment.has_sn = true;
        segment.rescan();

        EXPECT_EQ(2, segment.get_SN());
        ASSERT_EQ(3, segment.get_payload_size());

        to_buffer_direct(cmp, "001122");
        EXPECT_EQ(0, std::memcmp(segment.payload, cmp, 3));

        EXPECT_EQ(pdcp.get_header_size() + 10 + 8, tx_info.out_allocated);
    }
}
