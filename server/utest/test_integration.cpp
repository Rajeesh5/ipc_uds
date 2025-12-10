/**
 * @file test_integration.cpp
 * @brief Integration tests (require running server)
 */

#include "ipc_sync/Channel.hpp"
#include "ipc_sync/CalculatorClient.hpp"
#include "ipc_sync/TimeClient.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <ctime>

using namespace ipc_demo;

class IntegrationTest : public ::testing::Test {
protected:
    std::shared_ptr<Channel> channel_;
    std::string socket_path_ = Protocol::UDS_PATH;
    
    void SetUp() override {
        channel_ = std::make_shared<Channel>(socket_path_, 5000);
        
        if (!channel_->IsConnected()) {
            GTEST_SKIP() << "Server not running on " << socket_path_;
        }
    }
};

TEST_F(IntegrationTest, CalculatorAddition) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Add(10.5, 5.3);
    ASSERT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_DOUBLE_EQ(result.value, 15.8);
}

TEST_F(IntegrationTest, CalculatorSubtraction) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Subtract(20.0, 8.5);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 11.5);
}

TEST_F(IntegrationTest, CalculatorMultiplication) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Multiply(7.5, 4.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 30.0);
}

TEST_F(IntegrationTest, CalculatorDivision) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Divide(100.0, 5.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 20.0);
}

TEST_F(IntegrationTest, CalculatorDivisionByZero) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Divide(42.0, 0.0);
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
    EXPECT_NE(result.error_message.find("zero"), std::string::npos);
}

TEST_F(IntegrationTest, CalculatorAllOperations) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    // Addition
    auto result = calculator->Add(10.5, 5.3);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 15.8);
    
    // Subtraction
    result = calculator->Subtract(20.0, 8.5);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 11.5);
    
    // Multiplication
    result = calculator->Multiply(7.5, 4.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 30.0);
    
    // Division
    result = calculator->Divide(100.0, 5.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 20.0);
}

TEST_F(IntegrationTest, TimeService) {
    auto timeClient = std::make_unique<TimeClient>(channel_);
    
    auto result = timeClient->GetCurrentTime();
    
    ASSERT_TRUE(result.success) << "Error: " << result.error_message;
    EXPECT_FALSE(result.timestamp.empty());
    EXPECT_GT(result.unix_timestamp, 0);
    
    // Verify timestamp is reasonable (within 10 seconds of now)
    time_t now = time(nullptr);
    EXPECT_NEAR(result.unix_timestamp, now, 10);
}

TEST_F(IntegrationTest, MultipleServicesOnSameChannel) {
    auto calculator = std::make_unique<Calculator>(channel_);
    auto timeClient = std::make_unique<TimeClient>(channel_);
    
    // Use calculator
    auto calc_result = calculator->Add(5.0, 3.0);
    ASSERT_TRUE(calc_result.success);
    EXPECT_DOUBLE_EQ(calc_result.value, 8.0);
    
    // Use time service
    auto time_result = timeClient->GetCurrentTime();
    ASSERT_TRUE(time_result.success);
    
    // Use calculator again
    calc_result = calculator->Multiply(4.0, 2.0);
    ASSERT_TRUE(calc_result.success);
    EXPECT_DOUBLE_EQ(calc_result.value, 8.0);
    
    // Use time service again
    time_result = timeClient->GetCurrentTime();
    ASSERT_TRUE(time_result.success);
}

TEST_F(IntegrationTest, AlternatingServices) {
    auto calculator = std::make_unique<Calculator>(channel_);
    auto timeClient = std::make_unique<TimeClient>(channel_);
    
    for (int i = 0; i < 10; ++i) {
        auto calc_result = calculator->Add(static_cast<double>(i), 1.0);
        ASSERT_TRUE(calc_result.success);
        
        auto time_result = timeClient->GetCurrentTime();
        ASSERT_TRUE(time_result.success);
    }
}

TEST_F(IntegrationTest, StressTest) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    constexpr int NUM_OPERATIONS = 1000;
    int success_count = 0;
    
    for (int i = 0; i < NUM_OPERATIONS; ++i) {
        auto result = calculator->Add(static_cast<double>(i), 1.0);
        if (result.success) {
            EXPECT_DOUBLE_EQ(result.value, static_cast<double>(i) + 1.0);
            success_count++;
        }
    }
    
    EXPECT_EQ(success_count, NUM_OPERATIONS);
}

TEST_F(IntegrationTest, LargeNumbers) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Multiply(1e100, 2.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 2e100);
}

TEST_F(IntegrationTest, NegativeNumbers) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Add(-15.5, 20.3);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 4.8);
    
    result = calculator->Subtract(-10.0, -5.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, -5.0);
}

TEST_F(IntegrationTest, ZeroValues) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    auto result = calculator->Add(0.0, 0.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 0.0);
    
    result = calculator->Multiply(100.0, 0.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 0.0);
}

TEST_F(IntegrationTest, ConcurrentClientsToServer) {
    constexpr int NUM_CLIENTS = 10;
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    
    for (int i = 0; i < NUM_CLIENTS; ++i) {
        threads.emplace_back([this, &success_count, i]() {
            auto client_channel = std::make_shared<Channel>(socket_path_, 5000);
            
            if (client_channel->IsConnected()) {
                auto calculator = std::make_unique<Calculator>(client_channel);
                
                for (int j = 0; j < 50; ++j) {
                    auto result = calculator->Add(static_cast<double>(i), static_cast<double>(j));
                    if (result.success) {
                        success_count++;
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, NUM_CLIENTS * 50);
}

TEST_F(IntegrationTest, SequentialOperations) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    // Chain of operations
    double value = 0.0;
    
    auto result = calculator->Add(value, 10.0);
    ASSERT_TRUE(result.success);
    value = result.value; // 10.0
    
    result = calculator->Multiply(value, 2.0);
    ASSERT_TRUE(result.success);
    value = result.value; // 20.0
    
    result = calculator->Subtract(value, 5.0);
    ASSERT_TRUE(result.success);
    value = result.value; // 15.0
    
    result = calculator->Divide(value, 3.0);
    ASSERT_TRUE(result.success);
    value = result.value; // 5.0
    
    EXPECT_DOUBLE_EQ(value, 5.0);
}

TEST_F(IntegrationTest, ErrorRecovery) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    // Cause an error
    auto result = calculator->Divide(10.0, 0.0);
    EXPECT_FALSE(result.success);
    
    // Should still work after error
    result = calculator->Add(5.0, 3.0);
    ASSERT_TRUE(result.success);
    EXPECT_DOUBLE_EQ(result.value, 8.0);
}

TEST_F(IntegrationTest, MultipleTimeRequests) {
    auto timeClient = std::make_unique<TimeClient>(channel_);
    
    for (int i = 0; i < 10; ++i) {
        auto result = timeClient->GetCurrentTime();
        ASSERT_TRUE(result.success);
        EXPECT_FALSE(result.timestamp.empty());
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

TEST_F(IntegrationTest, RapidFireOperations) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    for (int i = 0; i < 100; ++i) {
        auto result = calculator->Add(1.0, 1.0);
        ASSERT_TRUE(result.success);
        EXPECT_DOUBLE_EQ(result.value, 2.0);
        // No delay - rapid fire
    }
}

TEST_F(IntegrationTest, MixedOperationTypes) {
    auto calculator = std::make_unique<Calculator>(channel_);
    
    std::vector<std::string> operations = {"add", "sub", "mul", "div"};
    
    for (int i = 0; i < 20; ++i) {
        int op_type = i % 4;
        Calculator::Result result;
        
        switch (op_type) {
            case 0:
                result = calculator->Add(10.0, 5.0);
                EXPECT_DOUBLE_EQ(result.value, 15.0);
                break;
            case 1:
                result = calculator->Subtract(10.0, 5.0);
                EXPECT_DOUBLE_EQ(result.value, 5.0);
                break;
            case 2:
                result = calculator->Multiply(10.0, 5.0);
                EXPECT_DOUBLE_EQ(result.value, 50.0);
                break;
            case 3:
                result = calculator->Divide(10.0, 5.0);
                EXPECT_DOUBLE_EQ(result.value, 2.0);
                break;
        }
        
        ASSERT_TRUE(result.success);
    }
}
