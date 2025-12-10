/**
 * @file test_multithreading.cpp
 * @brief Multithreading and concurrency tests
 */

#include "ServiceManager.hpp"
#include "CalculatorService.hpp"
#include "TimeService.hpp"
#include "ipc_sync/Channel.hpp"
#include "ipc_sync/CalculatorClient.hpp"
#include "ipc_sync/TimeClient.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

using namespace ipc_demo;

class MultiThreadingTest : public ::testing::Test {
protected:
    std::unique_ptr<ServiceManager> manager_;
    
    void SetUp() override {
        manager_ = std::make_unique<ServiceManager>();
        auto calc_service = std::make_shared<CalculatorService>();
        auto time_service = std::make_shared<TimeService>();
        manager_->RegisterService(calc_service);
        manager_->RegisterService(time_service);
    }
};

TEST_F(MultiThreadingTest, ConcurrentServiceExecution) {
    constexpr int NUM_THREADS = 10;
    constexpr int OPERATIONS_PER_THREAD = 100;
    
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &success_count, i]() {
            std::vector<uint8_t> input(256);
            std::vector<uint8_t> output(256);
            
            for (int j = 0; j < OPERATIONS_PER_THREAD; ++j) {
                ByteBuffer req(input.data(), input.size());
                req.PutByte(0x01); // Add operation
                req.PutDouble(static_cast<double>(i));
                req.PutDouble(static_cast<double>(j));
                
                size_t resp_len = manager_->ExecuteService(
                    0x1000,
                    input.data(), req.Position(),
                    output.data(), output.size()
                );
                
                if (resp_len > 0) {
                    success_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, NUM_THREADS * OPERATIONS_PER_THREAD);
}

TEST_F(MultiThreadingTest, ConcurrentDifferentServices) {
    constexpr int NUM_THREADS = 10;
    std::atomic<int> calc_success{0};
    std::atomic<int> time_success{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &calc_success, &time_success, i]() {
            std::vector<uint8_t> input(256);
            std::vector<uint8_t> output(256);
            
            // Execute calculator service
            ByteBuffer calc_req(input.data(), input.size());
            calc_req.PutByte(0x01);
            calc_req.PutDouble(10.0);
            calc_req.PutDouble(5.0);
            
            size_t calc_resp = manager_->ExecuteService(
                0x1000, input.data(), calc_req.Position(),
                output.data(), output.size()
            );
            if (calc_resp > 0) calc_success++;
            
            // Execute time service
            ByteBuffer time_req(input.data(), input.size());
            time_req.PutByte(0x01);
            
            size_t time_resp = manager_->ExecuteService(
                0x2000, input.data(), time_req.Position(),
                output.data(), output.size()
            );
            if (time_resp > 0) time_success++;
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(calc_success, NUM_THREADS);
    EXPECT_EQ(time_success, NUM_THREADS);
}

TEST_F(MultiThreadingTest, StressTestManyThreads) {
    constexpr int NUM_THREADS = 50;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &success_count, &error_count]() {
            std::vector<uint8_t> input(256);
            std::vector<uint8_t> output(256);
            
            for (int j = 0; j < 20; ++j) {
                ByteBuffer req(input.data(), input.size());
                req.PutByte(0x01);
                req.PutDouble(1.0);
                req.PutDouble(2.0);
                
                size_t resp_len = manager_->ExecuteService(
                    0x1000,
                    input.data(), req.Position(),
                    output.data(), output.size()
                );
                
                if (resp_len > 0) {
                    success_count++;
                } else {
                    error_count++;
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(success_count, NUM_THREADS * 20);
    EXPECT_EQ(error_count, 0);
}

TEST_F(MultiThreadingTest, RaceConditionOnServiceManager) {
    constexpr int NUM_THREADS = 20;
    std::vector<std::thread> threads;
    std::atomic<int> operation_count{0};
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &operation_count, i]() {
            std::vector<uint8_t> input(256);
            std::vector<uint8_t> output(256);
            
            // Mix of different operations
            for (int j = 0; j < 50; ++j) {
                ByteBuffer req(input.data(), input.size());
                uint8_t op = (j % 4) + 1; // Cycle through operations
                req.PutByte(op);
                req.PutDouble(static_cast<double>(i * 10));
                req.PutDouble(static_cast<double>(j + 1));
                
                manager_->ExecuteService(
                    0x1000,
                    input.data(), req.Position(),
                    output.data(), output.size()
                );
                
                operation_count++;
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(operation_count, NUM_THREADS * 50);
}

// Integration test with Channel (requires running server)
class MultiThreadingIntegrationTest : public ::testing::Test {
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

TEST_F(MultiThreadingIntegrationTest, SharedChannelAccess) {
    auto channel = std::make_shared<Channel>(socket_path_, 5000);
    
    ASSERT_TRUE(channel->IsConnected());
    
    constexpr int NUM_THREADS = 5;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([channel, &success_count]() {
            auto calculator = std::make_unique<Calculator>(channel);
            
            for (int j = 0; j < 20; ++j) {
                auto result = calculator->Add(1.0, 2.0);
                if (result.success) {
                    success_count++;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(success_count, 0);
}

TEST_F(MultiThreadingIntegrationTest, MultipleChannelsConcurrent) {
    constexpr int NUM_THREADS = 10;
    std::atomic<int> success_count{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &success_count]() {
            auto channel = std::make_shared<Channel>(socket_path_, 5000);
            
            if (channel->IsConnected()) {
                auto calculator = std::make_unique<Calculator>(channel);
                
                for (int j = 0; j < 10; ++j) {
                    auto result = calculator->Multiply(2.0, 3.0);
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
    
    EXPECT_EQ(success_count, NUM_THREADS * 10);
}

TEST_F(MultiThreadingIntegrationTest, MixedServicesConcurrent) {
    constexpr int NUM_THREADS = 8;
    std::atomic<int> calc_success{0};
    std::atomic<int> time_success{0};
    std::vector<std::thread> threads;
    
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, &calc_success, &time_success, i]() {
            auto channel = std::make_shared<Channel>(socket_path_, 5000);
            
            if (channel->IsConnected()) {
                auto calculator = std::make_unique<Calculator>(channel);
                auto timeClient = std::make_unique<TimeClient>(channel);
                
                // Alternate between services
                for (int j = 0; j < 10; ++j) {
                    if (j % 2 == 0) {
                        auto calc_result = calculator->Add(
                            static_cast<double>(i),
                            static_cast<double>(j)
                        );
                        if (calc_result.success) calc_success++;
                    } else {
                        auto time_result = timeClient->GetCurrentTime();
                        if (time_result.success) time_success++;
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_GT(calc_success, 0);
    EXPECT_GT(time_success, 0);
}
