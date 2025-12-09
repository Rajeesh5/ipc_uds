/**
 * @file IService.hpp
 * @brief Service interface for RPC handlers
 * 
 * Defines the contract that all services must follow.
 * Open/Closed Principle: Easy to add new services without modifying framework.
 */

#ifndef IPC_DEMO_ISERVICE_HPP
#define IPC_DEMO_ISERVICE_HPP

#include <cstdint>
#include <cstddef>
#include <string>

namespace ipc_demo {

/**
 * @class IService
 * @brief Abstract interface for RPC service implementations
 * 
 * Services implement this interface to handle specific RPC requests.
 * Each service is identified by unique request/response routine IDs.
 */
class IService {
public:
    virtual ~IService() = default;

    /**
     * @brief Get the request routine ID this service handles
     * @return Unique routine ID for requests
     */
    virtual uint32_t GetRequestRoutineId() const = 0;

    /**
     * @brief Get the response routine ID this service produces
     * @return Unique routine ID for responses
     */
    virtual uint32_t GetResponseRoutineId() const = 0;

    /**
     * @brief Execute the service logic
     * 
     * @param input Raw input data (already deserialized frame, starting after VERSION byte)
     * @param input_len Length of input data
     * @param output Buffer to write response (including full frame)
     * @param output_len Maximum output buffer size
     * @return Number of bytes written to output, or 0 on error
     */
    virtual size_t Execute(const uint8_t* input, size_t input_len,
                          uint8_t* output, size_t output_len) = 0;

    /**
     * @brief Get service name for logging
     * @return Human-readable service name
     */
    virtual std::string GetName() const = 0;
};

} // namespace ipc_demo

#endif // IPC_DEMO_ISERVICE_HPP
