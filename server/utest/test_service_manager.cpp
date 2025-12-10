/**
 * @file test_service_manager.cpp
 * @brief Unit tests for ServiceManager
 */

#include "ServiceManager.hpp"
#include "IService.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>

using namespace ipc_demo;

// Mock service for testing
class MockService : public IService {
public:
    explicit MockService(uint32_t req_id, uint32_t resp_id, const std::string& name = "MockService")
        : req_id_(req_id), resp_id_(resp_id), name_(name), execute_count_(0) {}
    
    uint32_t GetRequestRoutineId() const override { return req_id_; }
    uint32_t GetResponseRoutineId() const override { return resp_id_; }
    std::string GetName() const override { return name_; }
    
    size_t Execute(const uint8_t* input, size_t input_len, uint8_t* output, size_t output_size) override {
        execute_count_++;
        
        // Build simple response frame
        ByteBuffer resp(output, output_size);
        resp.PutByte(Protocol::START_BYTE);
        resp.PutInt(0); // Placeholder for length
        resp.PutInt(GetResponseRoutineId());
        resp.PutByte(Protocol::VERSION);
        resp.PutByte(0x42); // Test payload
        resp.PutByte(Protocol::END_BYTE);
        
        // Update length
        size_t frame_len = resp.Position();
        resp.SetPosition(1);
        resp.PutInt(static_cast<uint32_t>(frame_len));
        
        return frame_len;
    }
    
    int GetExecuteCount() const { return execute_count_; }
    
private:
    uint32_t req_id_;
    uint32_t resp_id_;
    std::string name_;
    int execute_count_;
};

class ServiceManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<ServiceManager> manager_;
    
    void SetUp() override {
        manager_ = std::make_unique<ServiceManager>();
    }
};

TEST_F(ServiceManagerTest, RegisterService) {
    auto service = std::make_shared<MockService>(0x1000, 0x1001);
    
    EXPECT_TRUE(manager_->RegisterService(service));
    EXPECT_TRUE(manager_->IsRoutinePresent(0x1000));
    EXPECT_EQ(manager_->GetServiceCount(), 1u);
}

TEST_F(ServiceManagerTest, RegisterMultipleServices) {
    auto service1 = std::make_shared<MockService>(0x1000, 0x1001, "Service1");
    auto service2 = std::make_shared<MockService>(0x2000, 0x2001, "Service2");
    auto service3 = std::make_shared<MockService>(0x3000, 0x3001, "Service3");
    
    EXPECT_TRUE(manager_->RegisterService(service1));
    EXPECT_TRUE(manager_->RegisterService(service2));
    EXPECT_TRUE(manager_->RegisterService(service3));
    
    EXPECT_EQ(manager_->GetServiceCount(), 3u);
    EXPECT_TRUE(manager_->IsRoutinePresent(0x1000));
    EXPECT_TRUE(manager_->IsRoutinePresent(0x2000));
    EXPECT_TRUE(manager_->IsRoutinePresent(0x3000));
}

TEST_F(ServiceManagerTest, RegisterDuplicateServiceFails) {
    auto service1 = std::make_shared<MockService>(0x1000, 0x1001);
    auto service2 = std::make_shared<MockService>(0x1000, 0x1002);
    
    EXPECT_TRUE(manager_->RegisterService(service1));
    EXPECT_FALSE(manager_->RegisterService(service2)); // Duplicate routine ID
    EXPECT_EQ(manager_->GetServiceCount(), 1u);
}

TEST_F(ServiceManagerTest, ExecuteService) {
    auto service = std::make_shared<MockService>(0x1000, 0x1001);
    manager_->RegisterService(service);
    
    uint8_t input[256] = {0};
    uint8_t output[256] = {0};
    
    size_t result = manager_->ExecuteService(0x1000, input, sizeof(input), output, sizeof(output));
    
    EXPECT_GT(result, 0u);
    EXPECT_EQ(service->GetExecuteCount(), 1);
}

TEST_F(ServiceManagerTest, ExecuteNonExistentService) {
    uint8_t input[256] = {0};
    uint8_t output[256] = {0};
    
    size_t result = manager_->ExecuteService(0x9999, input, sizeof(input), output, sizeof(output));
    
    EXPECT_EQ(result, 0u);
}

TEST_F(ServiceManagerTest, ExecuteMultipleTimesOnSameService) {
    auto service = std::make_shared<MockService>(0x1000, 0x1001);
    manager_->RegisterService(service);
    
    uint8_t input[256] = {0};
    uint8_t output[256] = {0};
    
    for (int i = 0; i < 10; ++i) {
        size_t result = manager_->ExecuteService(0x1000, input, sizeof(input), output, sizeof(output));
        EXPECT_GT(result, 0u);
    }
    
    EXPECT_EQ(service->GetExecuteCount(), 10);
}

TEST_F(ServiceManagerTest, GetAllServices) {
    auto service1 = std::make_shared<MockService>(0x1000, 0x1001, "Service1");
    auto service2 = std::make_shared<MockService>(0x2000, 0x2001, "Service2");
    
    manager_->RegisterService(service1);
    manager_->RegisterService(service2);
    
    auto services = manager_->GetAllServices();
    EXPECT_EQ(services.size(), 2u);
}

TEST_F(ServiceManagerTest, Clear) {
    auto service1 = std::make_shared<MockService>(0x1000, 0x1001);
    auto service2 = std::make_shared<MockService>(0x2000, 0x2001);
    
    manager_->RegisterService(service1);
    manager_->RegisterService(service2);
    
    EXPECT_EQ(manager_->GetServiceCount(), 2u);
    
    manager_->Clear();
    
    EXPECT_EQ(manager_->GetServiceCount(), 0u);
    EXPECT_FALSE(manager_->IsRoutinePresent(0x1000));
    EXPECT_FALSE(manager_->IsRoutinePresent(0x2000));
}

TEST_F(ServiceManagerTest, IsRoutineNotPresent) {
    EXPECT_FALSE(manager_->IsRoutinePresent(0x9999));
}

TEST_F(ServiceManagerTest, ConcurrentRegistration) {
    constexpr int NUM_THREADS = 10;
    std::vector<std::thread> threads;
    
    // Try to register services concurrently
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back([this, i]() {
            auto service = std::make_shared<MockService>(0x1000 + i * 0x100, 0x1001 + i * 0x100);
            manager_->RegisterService(service);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(manager_->GetServiceCount(), NUM_THREADS);
}
