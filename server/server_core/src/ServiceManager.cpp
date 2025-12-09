/**
 * @file ServiceManager.cpp
 * @brief Implementation of ServiceManager
 */

#include "ServiceManager.hpp"
#include <iostream>

namespace ipc_demo {

bool ServiceManager::RegisterService(std::shared_ptr<IService> service) {
    if (!service) {
        std::cerr << "[ServiceManager] Cannot register null service" << std::endl;
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    uint32_t routine_id = service->GetRequestRoutineId();
    
    // Check if already registered
    if (services_.find(routine_id) != services_.end()) {
        std::cerr << "[ServiceManager] Service with routine ID 0x"
                  << std::hex << routine_id << std::dec
                  << " already registered" << std::endl;
        return false;
    }

    services_[routine_id] = service;
    
    std::cout << "[ServiceManager] Registered service: " << service->GetName()
              << " (Request ID: 0x" << std::hex << routine_id << std::dec << ")"
              << std::endl;
    
    return true;
}

bool ServiceManager::IsRoutinePresent(uint32_t routine_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_.find(routine_id) != services_.end();
}

size_t ServiceManager::ExecuteService(uint32_t routine_id,
                                     const uint8_t* input, size_t input_len,
                                     uint8_t* output, size_t output_len) {
    std::shared_ptr<IService> service;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(routine_id);
        if (it == services_.end()) {
            std::cerr << "[ServiceManager] No service found for routine ID 0x"
                      << std::hex << routine_id << std::dec << std::endl;
            return 0;
        }
        service = it->second;
    }

    // Execute outside of lock to allow concurrent service execution
    try {
        return service->Execute(input, input_len, output, output_len);
    } catch (const std::exception& e) {
        std::cerr << "[ServiceManager] Exception in service " << service->GetName()
                  << ": " << e.what() << std::endl;
        return 0;
    }
}

std::vector<std::shared_ptr<IService>> ServiceManager::GetAllServices() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::shared_ptr<IService>> result;
    result.reserve(services_.size());
    
    for (const auto& [id, service] : services_) {
        result.push_back(service);
    }
    
    return result;
}

size_t ServiceManager::GetServiceCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return services_.size();
}

void ServiceManager::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::cout << "[ServiceManager] Clearing " << services_.size() << " services" << std::endl;
    services_.clear();
}

} // namespace ipc_demo
