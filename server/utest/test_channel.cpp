/**
 * @file test_channel.cpp
 * @brief Unit tests for Channel (requires running server for integration tests)
 */

#include "ipc_sync/Channel.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace ipc_demo;

// Note: These tests require a running server for full functionality
// Some tests will be skipped if server is not available
class ChannelTest : public ::testing::Test {
protected:
    std::string socket_path_ = "/tmp/test_ipc_channel.sock";
    
    void SetUp() override {
        // Give time for any previous connections to close
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
};

TEST_F(ChannelTest, ConstructorWithDefaults) {
    auto channel = std::make_shared<Channel>(socket_path_);
    EXPECT_NE(channel, nullptr);
}

TEST_F(ChannelTest, ConstructorWithTimeout) {
    auto channel = std::make_shared<Channel>(socket_path_, 2000);
    EXPECT_NE(channel, nullptr);
}

TEST_F(ChannelTest, ConnectToNonExistentServer) {
    auto channel = std::make_shared<Channel>("/tmp/nonexistent_socket.sock", 100);
    
    // Should fail to connect but not crash
    EXPECT_FALSE(channel->IsConnected());
}

TEST_F(ChannelTest, DisconnectWithoutConnect) {
    auto channel = std::make_shared<Channel>(socket_path_, 100);
    
    // Should be able to disconnect even if not connected
    channel->Disconnect();
    EXPECT_FALSE(channel->IsConnected());
}

TEST_F(ChannelTest, MultipleDisconnects) {
    auto channel = std::make_shared<Channel>(socket_path_, 100);
    
    // Multiple disconnects should be safe
    channel->Disconnect();
    channel->Disconnect();
    channel->Disconnect();
    EXPECT_FALSE(channel->IsConnected());
}

TEST_F(ChannelTest, GetLastError) {
    auto channel = std::make_shared<Channel>("/tmp/nonexistent.sock", 100);
    
    // After failed connection attempt, should have error message
    if (!channel->IsConnected()) {
        std::string error = channel->GetLastError();
        // Error should be non-empty
        EXPECT_FALSE(error.empty());
    }
}

// Integration tests (require running server)
class ChannelIntegrationTest : public ::testing::Test {
protected:
    std::string socket_path_ = Protocol::UDS_PATH;
    
    bool IsServerRunning() {
        auto test_channel = std::make_shared<Channel>(socket_path_, 500);
        return test_channel->IsConnected();
    }
    
    void SetUp() override {
        if (!IsServerRunning()) {
            GTEST_SKIP() << "Server not running on " << socket_path_;
        }
    }
};

TEST_F(ChannelIntegrationTest, ConnectDisconnect) {
    auto channel = std::make_shared<Channel>(socket_path_, 1000);
    
    EXPECT_TRUE(channel->IsConnected());
    
    channel->Disconnect();
    EXPECT_FALSE(channel->IsConnected());
}

TEST_F(ChannelIntegrationTest, ReconnectAfterDisconnect) {
    auto channel = std::make_shared<Channel>(socket_path_, 1000);
    
    ASSERT_TRUE(channel->IsConnected());
    
    channel->Disconnect();
    EXPECT_FALSE(channel->IsConnected());
    
    EXPECT_TRUE(channel->Connect());
    EXPECT_TRUE(channel->IsConnected());
}

TEST_F(ChannelIntegrationTest, MultipleChannelsToSameServer) {
    auto channel1 = std::make_shared<Channel>(socket_path_, 1000);
    auto channel2 = std::make_shared<Channel>(socket_path_, 1000);
    auto channel3 = std::make_shared<Channel>(socket_path_, 1000);
    
    // All should be able to connect independently
    EXPECT_TRUE(channel1->IsConnected());
    EXPECT_TRUE(channel2->IsConnected());
    EXPECT_TRUE(channel3->IsConnected());
    
    // Disconnect one shouldn't affect others
    channel1->Disconnect();
    EXPECT_FALSE(channel1->IsConnected());
    EXPECT_TRUE(channel2->IsConnected());
    EXPECT_TRUE(channel3->IsConnected());
}

TEST_F(ChannelIntegrationTest, SequentialConnectDisconnect) {
    for (int i = 0; i < 10; ++i) {
        auto channel = std::make_shared<Channel>(socket_path_, 1000);
        EXPECT_TRUE(channel->IsConnected());
        channel->Disconnect();
        EXPECT_FALSE(channel->IsConnected());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

TEST_F(ChannelIntegrationTest, ConcurrentConnections) {
    constexpr int NUM_THREADS = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &success_count]() {
            auto channel = std::make_shared<Channel>(socket_path_, 2000);
            if (channel->IsConnected()) {
                success_count++;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                channel->Disconnect();
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, NUM_THREADS);
}

TEST_F(ChannelIntegrationTest, RapidConnectDisconnect) {
    auto channel = std::make_shared<Channel>(socket_path_, 1000);
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(channel->IsConnected());
        channel->Disconnect();
        EXPECT_FALSE(channel->IsConnected());
        EXPECT_TRUE(channel->Connect());
    }
}
