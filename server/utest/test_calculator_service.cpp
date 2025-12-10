/**
 * @file test_calculator_service.cpp
 * @brief Unit tests for CalculatorService
 */

#include "CalculatorService.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>

using namespace ipc_demo;

class CalculatorServiceTest : public ::testing::Test {
protected:
    std::unique_ptr<CalculatorService> service_;
    std::vector<uint8_t> input_buffer_;
    std::vector<uint8_t> output_buffer_;
    
    void SetUp() override {
        service_ = std::make_unique<CalculatorService>();
        input_buffer_.resize(1024);
        output_buffer_.resize(1024);
    }
    
    size_t BuildRequest(uint8_t op, double a, double b) {
        ByteBuffer buf(input_buffer_.data(), input_buffer_.size());
        buf.PutByte(op);
        buf.PutDouble(a);
        buf.PutDouble(b);
        return buf.Position();
    }
    
    void ParseResponse(size_t resp_len, uint8_t& status, double& result, std::string& error) {
        ByteBuffer buf(output_buffer_.data(), resp_len);
        
        buf.GetByte(); // START_BYTE
        buf.GetInt();  // frame_len
        buf.GetInt();  // routine_id
        buf.GetByte(); // version
        
        status = buf.GetByte();
        result = buf.GetDouble();
        error = buf.GetString();
    }
};

TEST_F(CalculatorServiceTest, RoutineIds) {
    EXPECT_EQ(service_->GetRequestRoutineId(), 0x1000u);
    EXPECT_EQ(service_->GetResponseRoutineId(), 0x1001u);
    EXPECT_EQ(service_->GetName(), "CalculatorService");
}

TEST_F(CalculatorServiceTest, Addition) {
    size_t req_len = BuildRequest(0x01, 10.5, 5.3);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00); // Success
    EXPECT_DOUBLE_EQ(result, 15.8);
    EXPECT_TRUE(error.empty());
}

TEST_F(CalculatorServiceTest, Subtraction) {
    size_t req_len = BuildRequest(0x02, 20.0, 8.5);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 11.5);
    EXPECT_TRUE(error.empty());
}

TEST_F(CalculatorServiceTest, Multiplication) {
    size_t req_len = BuildRequest(0x03, 7.5, 4.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 30.0);
    EXPECT_TRUE(error.empty());
}

TEST_F(CalculatorServiceTest, Division) {
    size_t req_len = BuildRequest(0x04, 100.0, 5.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 20.0);
    EXPECT_TRUE(error.empty());
}

TEST_F(CalculatorServiceTest, DivisionByZero) {
    size_t req_len = BuildRequest(0x04, 42.0, 0.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x01); // DivisionByZero
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("zero"), std::string::npos);
}

TEST_F(CalculatorServiceTest, InvalidOperation) {
    size_t req_len = BuildRequest(0xFF, 1.0, 2.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x02); // InvalidOperation
    EXPECT_FALSE(error.empty());
}

TEST_F(CalculatorServiceTest, NegativeNumbers) {
    size_t req_len = BuildRequest(0x01, -15.5, 20.3);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 4.8);
}

TEST_F(CalculatorServiceTest, SmallNumbers) {
    size_t req_len = BuildRequest(0x03, 0.5, 0.5);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 0.25);
}

TEST_F(CalculatorServiceTest, LargeNumbers) {
    size_t req_len = BuildRequest(0x03, 1e100, 2.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 2e100);
}

TEST_F(CalculatorServiceTest, ZeroOperands) {
    size_t req_len = BuildRequest(0x01, 0.0, 0.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_DOUBLE_EQ(result, 0.0);
}

TEST_F(CalculatorServiceTest, DivisionPrecision) {
    size_t req_len = BuildRequest(0x04, 1.0, 3.0);
    
    size_t resp_len = service_->Execute(
        input_buffer_.data(), req_len,
        output_buffer_.data(), output_buffer_.size()
    );
    
    ASSERT_GT(resp_len, 0u);
    
    uint8_t status;
    double result;
    std::string error;
    ParseResponse(resp_len, status, result, error);
    
    EXPECT_EQ(status, 0x00);
    EXPECT_NEAR(result, 0.33333333, 1e-7);
}
