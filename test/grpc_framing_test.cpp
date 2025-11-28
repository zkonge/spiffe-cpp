#include "http2_client.h"
#include <gtest/gtest.h>
#include <vector>

namespace spiffe {

TEST(GrpcFramingTest, PackMessage) {
    std::vector<uint8_t> message = {0x01, 0x02, 0x03};
    Buffer packed = GrpcFraming::pack_message(message);

    // Header: 1 byte compression flag (0), 4 bytes length (big endian)
    ASSERT_EQ(packed.size(), 5 + 3);
    EXPECT_EQ(packed[0], 0x00); // Compressed flag
    EXPECT_EQ(packed[1], 0x00);
    EXPECT_EQ(packed[2], 0x00);
    EXPECT_EQ(packed[3], 0x00);
    EXPECT_EQ(packed[4], 0x03); // Length
    EXPECT_EQ(packed[5], 0x01); // Data
}

TEST(GrpcFramingTest, UnpackMessage) {
    std::vector<uint8_t> packed = {
        0x00,                   // Compressed flag
        0x00, 0x00, 0x00, 0x03, // Length
        0x0A, 0x0B, 0x0C        // Data
    };

    Buffer message;
    bool success = GrpcFraming::unpack_message(packed, message);
    ASSERT_TRUE(success);
    ASSERT_EQ(message.size(), 3);
    EXPECT_EQ(message[0], 0x0A);
}

TEST(GrpcFramingTest, HasCompleteMessage) {
    std::vector<uint8_t> buffer = {
        0x00,                   // Compressed flag
        0x00, 0x00, 0x00, 0x03, // Length = 3
        0x0A, 0x0B              // Only 2 bytes of data
    };

    size_t msg_size;
    EXPECT_FALSE(GrpcFraming::has_complete_message(buffer, msg_size));

    buffer.push_back(0x0C); // Add missing byte
    EXPECT_TRUE(GrpcFraming::has_complete_message(buffer, msg_size));
    EXPECT_EQ(msg_size, 8); // 5 header + 3 data
}

TEST(GrpcFramingTest, HasCompleteMessageMultiple) {
    std::vector<uint8_t> buffer = {
        0x00, 0x00, 0x00, 0x00, 0x01, 0xAA, // Msg 1 (len 1)
        0x00, 0x00, 0x00, 0x00, 0x02, 0xBB  // Msg 2 (len 2, incomplete)
    };

    size_t msg_size;
    EXPECT_TRUE(GrpcFraming::has_complete_message(buffer, msg_size));
    EXPECT_EQ(msg_size, 6);
}

} // namespace spiffe
