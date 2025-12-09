/**
 * @file TimeService.cpp
 * @brief Implementation of Time service
 */

#include "TimeService.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace ipc_demo {

size_t TimeService::Execute(const uint8_t* input, size_t input_len,
                            uint8_t* output, size_t output_len) {
    try {
        // Parse request
        ByteBuffer request(const_cast<uint8_t*>(input), input_len);
        
        uint8_t op_byte = request.GetByte();
        Operation op = static_cast<Operation>(op_byte);

        std::cout << "[TimeService] Request: op=" << (int)op_byte << std::endl;

        // Execute operation
        std::string timestamp;
        int64_t unix_timestamp = 0;
        std::string error_msg;
        Status status = Status::InvalidOperation;

        if (op == Operation::GetTimestamp) {
            status = GetCurrentTimestamp(timestamp, unix_timestamp, error_msg);
        } else {
            error_msg = "Invalid operation code";
            status = Status::InvalidOperation;
        }

        // Build response frame
        ByteBuffer response(output, output_len);
        
        response.PutByte(Protocol::START_BYTE);
        response.PutInt(0); // Placeholder for length
        response.PutInt(GetResponseRoutineId());
        response.PutByte(Protocol::VERSION);
        response.PutByte(static_cast<uint8_t>(status));
        response.PutString(timestamp);
        response.PutLong(unix_timestamp);
        response.PutString(error_msg);
        response.PutByte(Protocol::END_BYTE);

        // Update length
        size_t total_len = response.Position();
        response.SetPosition(1);
        response.PutInt(total_len);

        std::cout << "[TimeService] Response: status=" << (int)status
                  << ", timestamp=" << timestamp 
                  << ", unix=" << unix_timestamp << std::endl;

        return total_len;

    } catch (const std::exception& e) {
        std::cerr << "[TimeService] Exception: " << e.what() << std::endl;
        
        // Build error response
        ByteBuffer response(output, output_len);
        response.PutByte(Protocol::START_BYTE);
        response.PutInt(0);
        response.PutInt(GetResponseRoutineId());
        response.PutByte(Protocol::VERSION);
        response.PutByte(static_cast<uint8_t>(Status::InvalidInput));
        response.PutString("");
        response.PutLong(0);
        response.PutString(std::string("Exception: ") + e.what());
        response.PutByte(Protocol::END_BYTE);

        size_t total_len = response.Position();
        response.SetPosition(1);
        response.PutInt(total_len);

        return total_len;
    }
}

TimeService::Status TimeService::GetCurrentTimestamp(
    std::string& timestamp, int64_t& unix_timestamp, std::string& error) {
    
    try {
        // Get current time
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        // Format timestamp as ISO 8601: YYYY-MM-DD HH:MM:SS.mmm
        std::tm tm_now;
        localtime_r(&time_t_now, &tm_now);
        
        std::ostringstream oss;
        oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S")
            << '.' << std::setfill('0') << std::setw(3) << ms.count();
        
        timestamp = oss.str();
        unix_timestamp = static_cast<int64_t>(time_t_now);
        error.clear();
        
        return Status::Success;
        
    } catch (const std::exception& e) {
        error = std::string("Failed to get timestamp: ") + e.what();
        timestamp.clear();
        unix_timestamp = 0;
        return Status::InvalidInput;
    }
}

} // namespace ipc_demo
