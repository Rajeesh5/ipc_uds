/**
 * @file test_protocol.cpp
 * @brief Unit tests for Protocol constants
 */

#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>

using namespace ipc_demo;

TEST(ProtocolTest, Constants) {
    EXPECT_EQ(Protocol::START_BYTE, 0x7E);
    EXPECT_EQ(Protocol::END_BYTE, 0x7F);
    EXPECT_EQ(Protocol::VERSION, 0x01);
    
    EXPECT_GT(Protocol::MAX_PACKET_SIZE, 0u);
    EXPECT_GT(Protocol::MIN_PACKET_SIZE, 0u);
    EXPECT_LT(Protocol::MIN_PACKET_SIZE, Protocol::MAX_PACKET_SIZE);
}

TEST(ProtocolTest, MinFrameSize) {
    size_t min_size = Protocol::GetMinFrameSize();
    
    // START(1) + LENGTH(4) + ROUTINE_ID(4) + VERSION(1) + END(1) = 11 bytes
    EXPECT_EQ(min_size, 11u);
}

TEST(ProtocolTest, SocketPath) {
    EXPECT_FALSE(Protocol::UDS_PATH.empty());
    EXPECT_EQ(Protocol::UDS_PATH.find("/tmp/"), 0u); // Starts with /tmp/
}

TEST(ProtocolTest, PacketSizeReasonable) {
    // MAX_PACKET_SIZE should be reasonable (not too small, not too large)
    EXPECT_GE(Protocol::MAX_PACKET_SIZE, 1024u);  // At least 1KB
    EXPECT_LE(Protocol::MAX_PACKET_SIZE, 10485760u); // At most 10MB
}

TEST(ProtocolTest, FrameSizeConsistency) {
    size_t min_frame = Protocol::GetMinFrameSize();
    EXPECT_LE(min_frame, Protocol::MAX_PACKET_SIZE);
    EXPECT_GE(min_frame, Protocol::MIN_PACKET_SIZE);
}
