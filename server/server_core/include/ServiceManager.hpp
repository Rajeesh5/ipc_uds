/**
 * @file ServiceManager.hpp
 * @brief Service registry and routing
 * 
 * Single Responsibility: Manages service registration and request routing
 */

#ifndef IPC_DEMO_SERVICE_MANAGER_HPP
#define IPC_DEMO_SERVICE_MANAGER_HPP

#include "IService.hpp"
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace ipc_demo {

/**
 * @class ServiceManager
 * @brief Registry for managing and routing RPC services
 * 
 * Thread-safe service registry that routes incoming requests
 * to appropriate service handlers based on routine ID.
 */
class ServiceManager {
public:
    ServiceManager() = default;
    ~ServiceManager() = default;

    // Disable copy/move
    ServiceManager(const ServiceManager&) = delete;
    ServiceManager& operator=(const ServiceManager&) = delete;
    ServiceManager(ServiceManager&&) = delete;
    ServiceManager& operator=(ServiceManager&&) = delete;

    /**
     * @brief Register a service
     * @param service Shared pointer to service implementation
     * @return true if registered successfully, false if routine ID already exists
     */
    bool RegisterService(std::shared_ptr<IService> service);

    /**
     * @brief Check if a routine ID is registered
     * @param routine_id Request routine ID
     * @return true if service exists
     */
    bool IsRoutinePresent(uint32_t routine_id) const;

    /**
     * @brief Execute service for given routine ID
     * @param routine_id Request routine ID
     * @param input Input data buffer
     * @param input_len Input data length
     * @param output Output data buffer
     * @param output_len Output buffer capacity
     * @return Number of bytes written to output, or 0 on error
     */
    size_t ExecuteService(uint32_t routine_id,
                         const uint8_t* input, size_t input_len,
                         uint8_t* output, size_t output_len);

    /**
     * @brief Get all registered services
     * @return Vector of service pointers
     */
    std::vector<std::shared_ptr<IService>> GetAllServices() const;

    /**
     * @brief Get number of registered services
     * @return Service count
     */
    size_t GetServiceCount() const;

    /**
     * @brief Clear all registered services
     */
    void Clear();

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint32_t, std::shared_ptr<IService>> services_;
};

} // namespace ipc_demo

#endif // IPC_DEMO_SERVICE_MANAGER_HPP
