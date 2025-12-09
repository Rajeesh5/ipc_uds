/**
 * @file TimeClient.hpp
 * @brief Client-side proxy for Time service (Pimpl interface)
 */
#ifndef IPC_SYNC_TIME_CLIENT_HPP
#define IPC_SYNC_TIME_CLIENT_HPP

#include "ipc_sync/Channel.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace ipc_demo {

/**
 * @class TimeClient
 * @brief Client proxy for Time service
 * 
 * Uses Pimpl idiom - all implementation is hidden in shared library
 */
class TimeClient {
public:
    struct TimeResult {
        bool success;
        std::string timestamp;
        int64_t unix_timestamp;
        std::string error_message;
    };

    /**
     * @brief Construct TimeClient proxy
     * @param channel Communication channel
     */
    explicit TimeClient(std::shared_ptr<Channel> channel);
    
    ~TimeClient();

    // Disable copy/move
    TimeClient(const TimeClient&) = delete;
    TimeClient& operator=(const TimeClient&) = delete;
    TimeClient(TimeClient&&) = delete;
    TimeClient& operator=(TimeClient&&) = delete;

    /**
     * @brief Get current server time
     */
    TimeResult GetCurrentTime();

private:
    // Opaque pointer to implementation
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ipc_demo

#endif // IPC_SYNC_TIME_CLIENT_HPP
