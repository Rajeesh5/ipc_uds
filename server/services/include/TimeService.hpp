/**
 * @file TimeService.hpp
 * @brief Time service implementation
 * 
 * Provides server time information via RPC
 */

#ifndef IPC_DEMO_TIME_SERVICE_HPP
#define IPC_DEMO_TIME_SERVICE_HPP

#include "IService.hpp"

namespace ipc_demo {

/**
 * @class TimeService
 * @brief Provides server time information via RPC
 * 
 * Operations:
 * - GetTimestamp: Returns current server time in human-readable format and Unix timestamp
 * 
 * Request Format (after version byte):
 *   [operation:byte]
 * 
 * Response Format (full frame):
 *   [START][LENGTH][RESPONSE_ID][VERSION][status:byte][timestamp:string][unix_timestamp:int64][error_msg:string][END]
 */
class TimeService : public IService {
public:
    // Operation codes
    enum class Operation : uint8_t {
        GetTimestamp = 0x01
    };

    // Status codes
    enum class Status : uint8_t {
        Success = 0x00,
        InvalidOperation = 0x01,
        InvalidInput = 0x02
    };

    TimeService() = default;
    ~TimeService() override = default;

    uint32_t GetRequestRoutineId() const override {
        return 0x2000; // Time service request ID
    }

    uint32_t GetResponseRoutineId() const override {
        return 0x2001; // Time service response ID
    }

    size_t Execute(const uint8_t* input, size_t input_len,
                  uint8_t* output, size_t output_len) override;

    std::string GetName() const override {
        return "TimeService";
    }

private:
    /**
     * @brief Get current server timestamp
     * @param timestamp Output formatted timestamp string
     * @param unix_timestamp Output Unix timestamp (seconds since epoch)
     * @param error Output error message
     * @return Status code
     */
    Status GetCurrentTimestamp(std::string& timestamp, int64_t& unix_timestamp, std::string& error);
};

} // namespace ipc_demo

#endif // IPC_DEMO_TIME_SERVICE_HPP
