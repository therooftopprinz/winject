#include <gtest/gtest.h>

#include <pdcp/pdcp_ul.hpp>
#include <pdcp/pdcp_dl.hpp>
#include <bfc/buffer.hpp>
#include <crc.hpp>
#include <cstring>
#include <random>

using namespace winject;

struct Test_pdcp_uldl : public ::testing::Test
{
};

struct header_t
{
    uint32_t sequence_number;
    uint32_t crc;
};

int random_range(int min, int max)
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

pdcp_ul_config_t make_ul_config(bool allow_segmentation, bool allow_reordering)
{
    pdcp_ul_config_t cfg{};
    cfg.allow_segmentation = allow_segmentation;
    cfg.allow_reordering   = allow_reordering;
    cfg.min_commit_size    = 4;
    return cfg;
}

pdcp_dl_config_t make_dl_config(bool allow_segmentation, bool allow_reordering)
{
    pdcp_dl_config_t cfg{};
    cfg.allow_segmentation = allow_segmentation;
    cfg.allow_reordering   = allow_reordering;
    return cfg;
}

bfc::buffer make_buffer(size_t size)
{
    std::byte* data = new std::byte[size];
    bfc::buffer buf(data, size);
    return buf;
}

bfc::buffer make_buffer(uint32_t sequence_number, size_t size)
{
    std::byte* data = new std::byte[size];
    bfc::buffer buf(data, size);

    for (size_t i = 0; i < size; ++i)
    {
        data[i] = static_cast<std::byte>(random_range(0, 255));
    }

    auto header = new (data) header_t{sequence_number, 0};
    crc32_04C11DB7 crc;
    header->crc = crc(data, size);
    return buf;
}

TEST_F(Test_pdcp_uldl, direct_ul_to_dl_should_work)
{
    pdcp_ul ul_sut(make_ul_config(true, true));
    pdcp_dl dl_sut(make_dl_config(true, true));

    ul_sut.on_frame_data(make_buffer(0, 1500));

    while (ul_sut.get_outstanding_bytes() > 0)
    {
        bfc::buffer buf = make_buffer(random_range(16, 100));
        auto written = ul_sut.write_pdcp(buf);
        ASSERT_GT(written, 0);
        ASSERT_TRUE(dl_sut.on_pdcp_data(bfc::buffer_view(buf.data(), written)));
        ASSERT_EQ(dl_sut.get_status(), pdcp_dl::STATUS_CODE_SUCCESS);
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

TEST_F(Test_pdcp_uldl, direct_dl_should_order_correctly)
{
    pdcp_ul ul_sut(make_ul_config(true, true));
    pdcp_dl dl_sut(make_dl_config(true, true));

    for (auto i=0u; i < 4; ++i)
    {
        auto frame_size = random_range(32, 1500);
        auto buffer = make_buffer(i, frame_size);
        auto header = reinterpret_cast<header_t*>(buffer.data());
        std::cout << "pushing: sn " << header->sequence_number << " crc " << header->crc << " size " << frame_size << std::endl;
        ul_sut.on_frame_data(std::move(buffer));
    }

    std::vector<bfc::sized_buffer> buffers;
    while (ul_sut.get_outstanding_bytes() > 0)
    {
        auto pdcp_size = random_range(7, 1500);
        bfc::buffer buf = make_buffer(pdcp_size);
        std::cout << "write_pdcp: size " << pdcp_size << std::endl;
        auto written = ul_sut.write_pdcp(buf);
        ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);
        buffers.emplace_back(bfc::sized_buffer(std::move(buf), written));
    }

    // Simulate random reordering of the buffers
    constexpr auto N_PASSES = 1u;
    std::vector<size_t> indices(buffers.size());
    std::iota(indices.begin(), indices.end(), 0);
    for (auto n=0u; n < N_PASSES; ++n)
    {
        for (uint32_t i=0; i < (buffers.size()-1); i++)
        {
            if ( random_range(0, 1) == 0)
            {
                std::swap(buffers[i], buffers[i+1]);
                std::swap(indices[i], indices[i+1]);
            }
        }
    }

    for (auto& index : indices)
    {
        std::cout << "reshuffle: sequence:  " << index << std::endl;
    }

    crc32_04C11DB7 crc;
    uint32_t expected_sequence_number = 0;
    for (auto& buffer : buffers)
    {
        dl_sut.on_pdcp_data(std::move(buffer));
        EXPECT_EQ(dl_sut.get_status(), pdcp_dl::STATUS_CODE_SUCCESS);
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

TEST_F(Test_pdcp_uldl, test_sequence012354)
{
    pdcp_ul ul_sut(make_ul_config(true, true));
    pdcp_dl dl_sut(make_dl_config(true, true));

    ul_sut.on_frame_data(make_buffer(0, 107));  ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);
    ul_sut.on_frame_data(make_buffer(1, 1267)); ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);
    ul_sut.on_frame_data(make_buffer(2, 1233)); ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);
    ul_sut.on_frame_data(make_buffer(3, 778));  ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);

    std::vector<bfc::buffer> tbuffers;
    tbuffers.emplace_back(make_buffer(70));
    tbuffers.emplace_back(make_buffer(1292));
    tbuffers.emplace_back(make_buffer(765));
    tbuffers.emplace_back(make_buffer(522));
    tbuffers.emplace_back(make_buffer(9));
    tbuffers.emplace_back(make_buffer(973));

    std::vector<bfc::sized_buffer> ubuffers;

    for (auto& buffer : tbuffers)
    {
        auto written = ul_sut.write_pdcp(buffer);
        ASSERT_EQ(ul_sut.get_status(), pdcp_ul::STATUS_CODE_SUCCESS);
        ubuffers.emplace_back(bfc::sized_buffer(std::move(buffer), written));
    }

    std::vector<bfc::sized_buffer> buffers;
    buffers.emplace_back(std::move(ubuffers[0]));
    buffers.emplace_back(std::move(ubuffers[1]));
    buffers.emplace_back(std::move(ubuffers[2]));
    buffers.emplace_back(std::move(ubuffers[3]));
    buffers.emplace_back(std::move(ubuffers[5]));
    buffers.emplace_back(std::move(ubuffers[4]));

    crc32_04C11DB7 crc;
    uint32_t expected_sequence_number = 0;
    uint32_t current_buffer = 0;
    for (auto& buffer : buffers)
    {
        std::cout << "on_pdcp_data: buffer " << current_buffer++ << std::endl;
        dl_sut.on_pdcp_data(std::move(buffer));
        ASSERT_EQ(dl_sut.get_status(), pdcp_dl::STATUS_CODE_SUCCESS);
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