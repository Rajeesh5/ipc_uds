/**
 * @file test_time_service.cpp
 * @brief Unit tests for TimeService
 */

#include "TimeService.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <ctime>

using namespace ipc_demo;

class TimeServiceTest : public ::testing::Test {
protected:
    std::unique_ptr<TimeService> service_;
    std::vector<uint8_t> input_buffer_;
    std::vector<uint8_t> output_buffer_;
    
    void SetUp() override {
        service_ = std::make_unique<TimeService>();
        input_buffer_.resize(1024);
        output_buffer_.resize(1024);
    }
};

TEST_F(TimeServiceTest, RoutineIds) {
    EXPECT_EQ(service_->GetRequestRoutineId(), 0x2000u);
    EXPECT_EQ(service_->GetResponseRoutineId(), 0x2001u);
    EXPECT_EQ(service_->GetName(), "TimeService");
}

TEST_F(TimeServiceTest, GetCurrentTime) {
    ByteBuffer req(input_buffer_.data(), input_buffer_.size());
    req.PutByte(0x01); // GetTimestamp operation
    
    time_t before = time(nullptr);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req.Position(),
        output_buffer_.data(), output_buffer_.size()
    );
    
    time_t after = time(nullptr);
    
    ASSERT_GT(resp_len, 0u);
    
    ByteBuffer resp(output_buffer_.data(), resp_len);
    resp.GetByte(); // START_BYTE
    resp.GetInt();  // frame_len
    resp.GetInt();  // routine_id
    resp.GetByte(); // version
    
    uint8_t status = resp.GetByte();
    std::string timestamp = resp.GetString();
    int64_t unix_timestamp = static_cast<int64_t>(resp.GetLong());
    std::string error = resp.GetString();
    
    EXPECT_EQ(status, 0x00); // Success
    EXPECT_FALSE(timestamp.empty());
    EXPECT_GT(unix_timestamp, 0);
    EXPECT_TRUE(error.empty());
    
    // Verify timestamp is within reasonable range
    EXPECT_GE(unix_timestamp, before);
    EXPECT_LE(unix_timestamp, after);
}

TEST_F(TimeServiceTest, InvalidOperation) {
    ByteBuffer req(input_buffer_.data(), input_buffer_.size());
    req.PutByte(0xFF); // Invalid operation
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req.Position(),
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    ByteBuffer resp(output_buffer_.data(), resp_len);
    resp.GetByte(); // START_BYTE
    resp.GetInt();  // frame_len
    resp.GetInt();  // routine_id
    resp.GetByte(); // version
    
    uint8_t status = resp.GetByte();
    
    EXPECT_EQ(status, 0x01); // InvalidOperation
}

TEST_F(TimeServiceTest, MultipleRequests) {
    // Test that multiple time requests work
    for (int i = 0; i < 5; ++i) {
        ByteBuffer req(input_buffer_.data(), input_buffer_.size());
        req.PutByte(0x01);
        
        size_t resp_len = service_->Execute(
            input_buffer_.data(), req.Position(),
            output_buffer_.data(), output_buffer_.size()
        );
        
        ASSERT_GT(resp_len, 0u);
        
        ByteBuffer resp(output_buffer_.data(), resp_len);
        resp.GetByte(); // START_BYTE
        resp.GetInt();  // frame_len
        resp.GetInt();  // routine_id
        resp.GetByte(); // version
        
        uint8_t status = resp.GetByte();
        EXPECT_EQ(status, 0x00);
    }
}

TEST_F(TimeServiceTest, TimestampFormat) {
    ByteBuffer req(input_buffer_.data(), input_buffer_.size());
    req.PutByte(0x01);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req.Position(),
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    ByteBuffer resp(output_buffer_.data(), resp_len);
    resp.GetByte(); // START_BYTE
    resp.GetInt();  // frame_len
    resp.GetInt();  // routine_id
    resp.GetByte(); // version
    
    uint8_t status = resp.GetByte();
    std::string timestamp = resp.GetString();
    
    EXPECT_EQ(status, 0x00);
    
    // Timestamp should contain expected components
    // Format: "YYYY-MM-DD HH:MM:SS"
    EXPECT_GE(timestamp.length(), 19u);
    EXPECT_NE(timestamp.find('-'), std::string::npos);
    EXPECT_NE(timestamp.find(':'), std::string::npos);
}
