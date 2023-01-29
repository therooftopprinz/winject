#include <gtest/gtest.h>
#include "LLC.hpp"
#include "MockPDCP.hpp"
#include "info_defs_matcher.hpp"
#include "buffer_utils.hpp"

struct Test_LLC : public testing::Test
{
    Test_LLC()
        : mock_pdcp(std::make_shared<MockPDCP>())
    {}

    llc_t prepare_data_pdu(uint8_t* buffer, lcid_t lcid, sn_t sn, llc_sz_t payload_size=5)
    {
        llc_t pdu(buffer, sizeof(buffer));
        pdu.set_D(true);
        pdu.set_A(false);
        pdu.set_SN(sn);
        pdu.set_LCID(lcid);
        pdu.set_payload_size(payload_size);
        return pdu;
    }

    rx_info_t trigger_data_rx(size_t slot_number, lcid_t lcid, sn_t sn, std::string pdcp_hex)
    {
        rx_info_t rx_info{rx_frame_info};
        rx_info.in_pdu.base = buffer;

        auto pdcp_data = to_buffer(pdcp_hex);

        llc_t rcv_pdu = prepare_data_pdu(buffer, lcid, sn, pdcp_data.size());
        memcpy(rcv_pdu.payload(), pdcp_data.data(), pdcp_data.size());
        rx_info.in_pdu.size = rcv_pdu.get_SIZE();

        rx_frame_info.slot_number = slot_number;
        sut->on_rx(rx_info);
        return rx_info;
    }

    tx_info_t trigger_tx(size_t slot_number, uint8_t* buffer, size_t size)
    {
        tx_frame_info.slot_number = slot_number;
        tx_info_t tx_info{tx_frame_info};
        tx_info.tx_available = false;
        tx_info.out_pdu.base = buffer;
        tx_info.out_pdu.size = size;
        sut->on_tx(tx_info);
        return tx_info;
    }

    std::shared_ptr<MockPDCP> mock_pdcp;
    std::shared_ptr<LLC> sut;
    frame_info_t tx_frame_info{};
    frame_info_t rx_frame_info{};
    uint8_t buffer[1024];
};

TEST_F(Test_LLC, should_not_allocate_llc_when_pdu_size_is_zero)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);
    EXPECT_CALL(*mock_pdcp, on_tx(testing::_)).Times(1);
    auto tx_info = trigger_tx(0, nullptr, 0);
    EXPECT_EQ(0, tx_info.out_allocated);
}

TEST_F(Test_LLC, should_allocate_ACK_first)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);

    EXPECT_CALL(*mock_pdcp, on_rx(testing::_)).Times(1);
    trigger_data_rx(0, 0, 0x3A, "AABBCCDDEE");
    
    size_t expected_pdu_size = llc_t::get_header_size() + sizeof(llc_payload_ack_t);
    EXPECT_CALL(*mock_pdcp, on_tx(testing::_)).Times(1);
    auto tx_info = trigger_tx(0, buffer, expected_pdu_size);
    ASSERT_EQ(tx_info.out_allocated, expected_pdu_size);

    llc_t send_pdu(buffer, expected_pdu_size);
    EXPECT_EQ(send_pdu.get_D(), true);
    EXPECT_EQ(send_pdu.get_A(), true);
    EXPECT_EQ(send_pdu.get_SN(), 0);
    EXPECT_EQ(send_pdu.get_LCID(), 0);
    EXPECT_EQ(send_pdu.get_SIZE(), expected_pdu_size);
    auto acks = (llc_payload_ack_t*) send_pdu.payload();
    EXPECT_EQ(acks[0].sn, 0x3A);
    EXPECT_EQ(acks[0].count, 1);
}

TEST_F(Test_LLC, should_combine_ACKs)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);

    EXPECT_CALL(*mock_pdcp, on_rx(testing::_)).Times(2);
    trigger_data_rx(0, 0, 0x01, "AABBCCDDEE");
    trigger_data_rx(0, 1, 0x02, "FF00112233");

    size_t expected_pdu_size = llc_t::get_header_size() + sizeof(llc_payload_ack_t);
    EXPECT_CALL(*mock_pdcp, on_tx(testing::_)).Times(1);
    auto tx_info = trigger_tx(0, buffer, expected_pdu_size);

    ASSERT_EQ(tx_info.out_allocated, expected_pdu_size);

    llc_t send_pdu(buffer, expected_pdu_size);
    EXPECT_EQ(send_pdu.get_D(), true);
    EXPECT_EQ(send_pdu.get_A(), true);
    EXPECT_EQ(send_pdu.get_SN(), 0);
    EXPECT_EQ(send_pdu.get_LCID(), 0);
    EXPECT_EQ(send_pdu.get_SIZE(), expected_pdu_size);
    auto acks = (llc_payload_ack_t*) send_pdu.payload();
    EXPECT_EQ(acks[0].sn, 0x01);
    EXPECT_EQ(acks[0].count, 2);
}

TEST_F(Test_LLC, should_allocate_ACKs_when_possible)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);

    EXPECT_CALL(*mock_pdcp, on_rx(testing::_)).Times(3);
    trigger_data_rx(0, 0, 0x01, "AABBCCDDEE");
    trigger_data_rx(0, 1, 0x03, "FF00112233");
    trigger_data_rx(0, 2, 0x05, "FF00112233");

    size_t expected_pdu_size = llc_t::get_header_size() + 3*sizeof(llc_payload_ack_t);
    EXPECT_CALL(*mock_pdcp, on_tx(testing::_))
        .Times(1);
    auto tx_info = trigger_tx(0, buffer, expected_pdu_size);

    ASSERT_EQ(tx_info.out_allocated, expected_pdu_size);

    llc_t send_pdu(buffer, expected_pdu_size);
    EXPECT_EQ(send_pdu.get_D(), true);
    EXPECT_EQ(send_pdu.get_A(), true);
    EXPECT_EQ(send_pdu.get_SN(), 0);
    EXPECT_EQ(send_pdu.get_LCID(), 0);
    EXPECT_EQ(send_pdu.get_SIZE(), expected_pdu_size);
    auto acks = (llc_payload_ack_t*) send_pdu.payload();
    EXPECT_EQ(acks[0].sn, 0x01);
    EXPECT_EQ(acks[0].count, 1);
    EXPECT_EQ(acks[1].sn, 0x03);
    EXPECT_EQ(acks[1].count, 1);
    EXPECT_EQ(acks[2].sn, 0x05);
    EXPECT_EQ(acks[2].count, 1);
}

TEST_F(Test_LLC, should_allocate_PDCP)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);

    auto str_data = "TEST";
    auto pdcp_writer = [&str_data](tx_info_t& tx_info) {
            if (tx_info.out_pdu.size >= 5)
            {
                strcpy((char*)tx_info.out_pdu.base, str_data);
                tx_info.out_pdu.size -= (strlen(str_data)+1);
                tx_info.out_allocated += (strlen(str_data)+1);
            }
        };
    EXPECT_CALL(*mock_pdcp, on_tx(testing::_)).
        WillRepeatedly(testing::Invoke(pdcp_writer));
    auto tx_info = trigger_tx(0, buffer, sizeof(buffer));
    EXPECT_EQ(llc_t::get_header_size() + strlen(str_data)+1, tx_info.out_allocated);
    llc_t llc(buffer, tx_info.out_allocated);
    EXPECT_EQ(0, llc.get_SN());
    EXPECT_EQ(0, llc.get_LCID());
    EXPECT_EQ(true, llc.get_D());
    ASSERT_EQ(strlen(str_data)+1, llc.get_payload_size());
    EXPECT_EQ(0, strcmp(str_data, (char*)llc.payload()));
}

TEST_F(Test_LLC, should_allocate_retx_PDCP)
{
    sut = std::make_shared<LLC>(mock_pdcp, 0, 3, 3);

    auto str_data = "TEST";
    auto pdcp_writer = [&str_data](tx_info_t& tx_info) {
            if (tx_info.out_pdu.size >= 5)
            {
                strcpy((char*)tx_info.out_pdu.base, str_data);
                tx_info.out_pdu.size -= (strlen(str_data)+1);
                tx_info.out_allocated += (strlen(str_data)+1);
            }
        };

    EXPECT_CALL(*mock_pdcp, on_tx(testing::_))
        .Times(4)
        .WillOnce(testing::Invoke(pdcp_writer))
        .WillRepeatedly(testing::Return());
    trigger_tx(0, buffer, sizeof(buffer));
    trigger_tx(1, nullptr, 0);
    trigger_tx(2, nullptr, 0);
    auto retx_info = trigger_tx(3, buffer, sizeof(buffer));
    llc_t llc(buffer, retx_info.out_allocated);
    EXPECT_EQ(1, llc.get_SN());
    EXPECT_EQ(0, llc.get_LCID());
    EXPECT_EQ(true, llc.get_D());
    ASSERT_EQ(strlen(str_data)+1, llc.get_payload_size());
    EXPECT_EQ(0, strcmp(str_data, (char*)llc.payload()));
}
