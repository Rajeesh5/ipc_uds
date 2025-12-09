/**
 * @file Protocol.hpp
 * @brief Protocol constants and definitions
 */

#ifndef IPC_SYNC_PROTOCOL_HPP
#define IPC_SYNC_PROTOCOL_HPP

#include <cstdint>
#include <string>

namespace ipc_demo {

/**
 * @namespace Protocol
 * @brief Protocol-level constants
 */
namespace Protocol {

    // Frame delimiters
    constexpr uint8_t START_BYTE = 0x7E;
    constexpr uint8_t END_BYTE = 0x7F;
    constexpr uint8_t VERSION = 0x01;

    // Buffer sizes
    constexpr size_t MAX_PACKET_SIZE = 8 * 1024;  // 8KB max packet
    constexpr size_t MIN_PACKET_SIZE = 11;         // Minimum valid packet

    // Timeout settings
    constexpr int CONNECTION_TIMEOUT_MS = 5000;    // 5 seconds
    constexpr int READ_TIMEOUT_MS = 3000;          // 3 seconds
    constexpr int INACTIVITY_TIMEOUT_SEC = 300;    // 5 minutes

    // UDS path
    const std::string UDS_PATH = "/tmp/ipc_demo.sock";

    // Retry configuration
    constexpr int MAX_RETRIES = 2;

    /**
     * @brief Calculate minimum frame size
     * START(1) + LENGTH(4) + ROUTINE_ID(4) + VERSION(1) + END(1) = 11 bytes
     */
    constexpr size_t GetMinFrameSize() {
        return 11;
    }

} // namespace Protocol

} // namespace ipc_demo

#endif // IPC_SYNC_PROTOCOL_HPP
