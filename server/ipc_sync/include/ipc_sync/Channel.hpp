/**
 * @file Channel.hpp
 * @brief Client-side channel for RPC communication (Pimpl interface)
 */
#ifndef IPC_SYNC_CHANNEL_HPP
#define IPC_SYNC_CHANNEL_HPP

#include <string>
#include <memory>
#include <cstdint>
#include <cstddef>

namespace ipc_demo {

/**
 * @class Channel
 * @brief Manages client-side UDS connection and RPC execution
 * 
 * Uses Pimpl idiom - all implementation is hidden in shared library
 * 
 * RAII Design:
 * - Constructor automatically connects (retries on first API call if fails)
 * - Destructor automatically disconnects (no manual cleanup needed)
 * - Auto-reconnects on timeout or connection loss
 * 
 * Usage:
 * @code
 * {
 *     auto channel = std::make_shared<Channel>("/tmp/server.sock");
 *     // No need to call Connect() - already connected!
 *     
 *     Calculator calc(channel);
 *     auto result = calc.Add(5, 3);  // Works even after server timeout
 *     
 *     // No need to call Disconnect() - destructor handles it
 * }
 * @endcode
 */
class Channel {
public:
    /**
     * @brief Construct channel and auto-connect to server
     * @param socket_path Path to UDS socket
     * @param timeout_ms Connection timeout in milliseconds
     * 
     * Note: If initial connection fails, the channel will retry
     *       automatically on the first API call (ExecuteRPC)
     */
    explicit Channel(const std::string& socket_path, int timeout_ms = 5000);
    
    /**
     * @brief Destructor - automatically disconnects (RAII)
     */
    ~Channel();

    // Disable copy/move
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;
    Channel(Channel&&) = delete;
    Channel& operator=(Channel&&) = delete;

    /**
     * @brief Connect or reconnect to server
     * @return true if connected
     * 
     * Note: You typically don't need to call this manually.
     *       Constructor auto-connects, and ExecuteRPC auto-reconnects.
     *       Exposed for explicit reconnection if needed.
     */
    bool Connect();

    /**
     * @brief Execute RPC call with automatic reconnection
     * @param routine_id Service routine ID
     * @param request_data Request payload
     * @param request_len Request payload length
     * @param response_buffer Buffer for response
     * @param response_buffer_size Size of response buffer
     * @param response_len Actual response length received
     * @return true if RPC succeeded
     * 
     * This method automatically:
     * - Checks connection status
     * - Reconnects if connection was lost (e.g., server timeout)
     * - Retries send once on connection failure
     */
    bool ExecuteRPC(uint32_t routine_id,
                    const uint8_t* request_data, size_t request_len,
                    uint8_t* response_buffer, size_t response_buffer_size,
                    size_t& response_len);

    /**
     * @brief Check if connected
     * @return true if connected
     */
    bool IsConnected() const;

    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string GetLastError() const;

    /**
     * @brief Disconnect from server
     * 
     * Note: You typically don't need to call this manually.
     *       Destructor auto-disconnects (RAII pattern).
     *       Exposed for explicit cleanup if needed.
     */
    void Disconnect();

private:
    // Opaque pointer to implementation
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ipc_demo

#endif // IPC_SYNC_CHANNEL_HPP
