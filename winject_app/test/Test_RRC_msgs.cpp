#include <cstdint>
#include <gtest/gtest.h>
#include "interface/rrc.hpp"
#include "buffer_utils.hpp"

uint8_t buffer[5000];


// using RRC_Message = std::variant<RRC_PullRequest,RRC_PullResponse,RRC_PushRequest,RRC_PushResponse,RRC_ActivateRequest,RRC_ActivateResponse>;
// TEST(Test_RRC_msgs, should_encode_decode_PullRequest)
// {
//     size_t encode_size = 0;
//     {
//         RRC rrc;
//         rrc.requestID = 4;
//         rrc.message = RRC_PullRequest{};
//         auto& msg = std::get<RRC_PullRequest>(rrc.message);
//         msg.includeLLCConfig = true;
//         msg.includePDCPConfig = false;
//         msg.lcid = 5;
//         cum::per_codec_ctx codec((std::byte*)buffer, sizeof(buffer));
//         encode_per(rrc, codec);
//         encode_size = sizeof(buffer) - codec.size();
//         printf("codec_size=%d\n", codec.size());
//         printf("encode_size=%d\n", encode_size);
//     }

//     {
//         RRC rrc;
//         cum::per_codec_ctx codec((std::byte*)buffer, encode_size);
//         decode_per(rrc, codec);
//         EXPECT_EQ(rrc.requestID, 4);
//         ASSERT_EQ(rrc.message.index(), 0);
//         auto& msg = std::get<RRC_PullRequest>(rrc.message);
//         EXPECT_EQ(msg.includeLLCConfig, true);
//         EXPECT_EQ(msg.includePDCPConfig, false);
//         EXPECT_EQ(msg.lcid, 5);
//     }
// }

// TEST(Test_RRC_msgs, should_encode_decode_PullResponse)
// {
//     size_t encode_size = 0;
//     {
//         RRC rrc;
//         rrc.requestID = 4;
//         rrc.message = RRC_PullResponse{};
//         auto& msg = std::get<RRC_PullResponse>(rrc.message);
//         cum::per_codec_ctx codec((std::byte*)buffer, sizeof(buffer));
//         encode_per(rrc, codec);
//         encode_size = sizeof(buffer) - codec.size();
//         printf("codec_size=%d\n", codec.size());
//         printf("encode_size=%d\n", encode_size);
//     }

//     {
//         RRC rrc;
//         cum::per_codec_ctx codec((std::byte*)buffer, encode_size);
//         decode_per(rrc, codec);
//         EXPECT_EQ(rrc.requestID, 4);
//         ASSERT_EQ(rrc.message.index(), 1);
//         // auto& msg = std::get<RRC_PullRequest>(rrc.message);
//     }
// }

