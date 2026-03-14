#include <gtest/gtest.h>

#include <pdcp/pdcp_ul.hpp>
#include <pdcp/pdcp_dl_cb.hpp>
#include <bfc/buffer.hpp>
#include <crc.hpp>
#include <cstring>
#include <random>

using namespace winject;

struct Test_pdcp_uldl_cb : public ::testing::Test
{
};

struct header_t
{
    uint32_t sequence_number;
    uint32_t crc;
};

namespace
{

int random_range_cb(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

pdcp_ul_config_t make_ul_config_cb(bool allow_segmentation, bool allow_reordering)
{
    pdcp_ul_config_t cfg{};
    cfg.allow_segmentation = allow_segmentation;
    cfg.allow_reordering   = allow_reordering;
    cfg.min_commit_size    = 4;
    return cfg;
}

pdcp_dl_cb_config_t make_dl_cb_config(bool allow_segmentation, bool allow_reordering, size_t reorder_buffer_len = 512)
{
    pdcp_dl_cb_config_t cfg{};
    cfg.allow_segmentation  = allow_segmentation;
    cfg.allow_reordering    = allow_reordering;
    cfg.reorder_buffer_len = reorder_buffer_len;
    return cfg;
}

bfc::buffer make_buffer_cb(size_t size)
{
    std::byte* data = new std::byte[size];
    bfc::buffer buf(data, size);
    return buf;
}

bfc::buffer make_buffer_cb(uint32_t sequence_number, size_t size)
{
    std::byte* data = new std::byte[size];
    bfc::buffer buf(data, size);

    for (size_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<std::byte>(random_range_cb(0, 255));
    }

    auto header = new (data) header_t{sequence_number, 0};
    crc32_04C11DB7 crc;
    header->crc = crc(data, size);
    return buf;
}

} // namespace

TEST_F(Test_pdcp_uldl_cb, direct_ul_to_dl_should_work)
{
    pdcp_ul ul_sut(make_ul_config_cb(true, true));
    pdcp_dl_cb dl_sut(make_dl_cb_config(true, true, 512));

    ul_sut.on_frame_data(make_buffer_cb(0, 1500));

    while (ul_sut.get_outstanding_bytes() > 0)
    {
        bfc::buffer buf = make_buffer_cb(random_range_cb(16, 100));
        auto written = ul_sut.write_pdcp(buf);
        ASSERT_GT(written, 0);
        ASSERT_TRUE(dl_sut.on_pdcp_data(bfc::buffer_view(buf.data(), written)));
        ASSERT_EQ(dl_sut.get_status(), pdcp_dl_cb::STATUS_CODE_SUCCESS);
    }

    crc32_04C11DB7 crc;

    uint32_t expected_sequence_number = 0;
    while (dl_sut.get_outstanding_packet() > 0)
    {
        auto buffer = dl_sut.pop();
        ASSERT_FALSE(buffer.empty());
        auto header = reinterpret_cast<header_t*>(buffer.data());
        auto expected_crc = header->crc;
        header->crc = 0;
        std::cout << "popped: sn " << header->sequence_number << " crc "<< expected_crc << std::endl;
        ASSERT_EQ(expected_crc, crc(buffer.data(), buffer.size()));
        ASSERT_EQ(header->sequence_number, expected_sequence_number++);
    }
}

TEST_F(Test_pdcp_uldl_cb, direct_dl_should_order_correctly)
{
    pdcp_ul ul_sut(make_ul_config_cb(true, true));
    pdcp_dl_cb dl_sut(make_dl_cb_config(true, true, 512));

    for (auto i=0u; i < 3; ++i)
    {
        auto buffer = make_buffer_cb(i, 1500);
        auto header = reinterpret_cast<header_t*>(buffer.data());
        std::cout << "pushing: sn " << header->sequence_number << " crc " << header->crc << std::endl;
        ul_sut.on_frame_data(std::move(buffer));
    }

    std::vector<bfc::sized_buffer> buffers;
    while (ul_sut.get_outstanding_bytes() > 0)
    {
        bfc::buffer buf = make_buffer_cb(750);
        auto written = ul_sut.write_pdcp(buf);
        ASSERT_GT(written, 0);
        buffers.emplace_back(bfc::sized_buffer(std::move(buf), written));
    }

    crc32_04C11DB7 crc;
    uint32_t expected_sequence_number = 0;
    for (auto& buffer : buffers)
    {
        dl_sut.on_pdcp_data(std::move(buffer));
        EXPECT_EQ(dl_sut.get_status(), pdcp_dl_cb::STATUS_CODE_SUCCESS);
        while (dl_sut.get_outstanding_packet())
        {
            auto buffer = dl_sut.pop();
            auto header = reinterpret_cast<header_t*>(buffer.data());
            auto expected_crc = header->crc;
            std::cout << "popped: sn " << header->sequence_number << " crc "<< expected_crc << std::endl;
            header->crc = 0;
            ASSERT_EQ(header->sequence_number, expected_sequence_number++);
            ASSERT_EQ(expected_crc, crc(buffer.data(), buffer.size()));
        }
    }
}
