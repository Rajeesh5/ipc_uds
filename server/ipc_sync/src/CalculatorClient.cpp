/**
 * @file Calculator.cpp
 * @brief Implementation of Calculator (stays on server side, part of shared library)
 */

#include "ipc_sync/CalculatorClient.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <stdexcept>
#include <iostream>

namespace ipc_demo {

enum class Operation : uint8_t {
    Add = 0x01,
    Subtract = 0x02,
    Multiply = 0x03,
    Divide = 0x04
};

// Private implementation (hidden from client)
struct Calculator::Impl {
    std::shared_ptr<Channel> channel_;
    static constexpr uint32_t REQUEST_ROUTINE_ID = 0x00001000;
    static constexpr uint32_t RESPONSE_ROUTINE_ID = 0x00001001;

    explicit Impl(std::shared_ptr<Channel> channel)
        : channel_(channel) {
        if (!channel_) {
            throw std::invalid_argument("Calculator: channel cannot be null");
        }
    }

    Calculator::Result ExecuteOperation(Operation op, double a, double b) {
        Calculator::Result result;
        result.success = false;
        result.value = 0.0;

        try {
            // Build request payload
            uint8_t request_data[1 + sizeof(double) * 2];
            ByteBuffer request_buf(request_data, sizeof(request_data));
            
            request_buf.PutByte(static_cast<uint8_t>(op));
            request_buf.PutDouble(a);
            request_buf.PutDouble(b);

            // Execute RPC
            uint8_t response_data[Protocol::MAX_PACKET_SIZE];
            size_t response_len = 0;

            bool rpc_ok = channel_->ExecuteRPC(
                REQUEST_ROUTINE_ID,
                request_data, request_buf.Position(),
                response_data, sizeof(response_data),
                response_len
            );

            if (!rpc_ok) {
                result.error_message = "RPC failed: " + channel_->GetLastError();
                return result;
            }

            // Parse response
            ByteBuffer response_buf(response_data, response_len);
            
            uint8_t start = response_buf.GetByte();
            if (start != Protocol::START_BYTE) {
                result.error_message = "Invalid response frame";
                return result;
            }

            uint32_t frame_len = response_buf.GetInt();
            uint32_t routine_id = response_buf.GetInt();
            
            if (routine_id != RESPONSE_ROUTINE_ID) {
                result.error_message = "Unexpected routine ID in response";
                return result;
            }

            uint8_t version = response_buf.GetByte();
            (void)version; // Protocol version for future use
            (void)frame_len; // Frame length already validated
            
            uint8_t status = response_buf.GetByte();
            double value = response_buf.GetDouble();
            std::string error_msg = response_buf.GetString();

            if (status == 0x00) { // Success
                result.success = true;
                result.value = value;
            } else {
                result.success = false;
                result.error_message = error_msg;
            }

            return result;

        } catch (const std::exception& e) {
            result.error_message = std::string("Exception: ") + e.what();
            return result;
        }
    }
};

// Public interface implementation
Calculator::Calculator(std::shared_ptr<Channel> channel)
    : pImpl_(std::make_unique<Impl>(channel)) {
}

Calculator::~Calculator() = default;

Calculator::Result Calculator::Add(double a, double b) {
    return pImpl_->ExecuteOperation(Operation::Add, a, b);
}

Calculator::Result Calculator::Subtract(double a, double b) {
    return pImpl_->ExecuteOperation(Operation::Subtract, a, b);
}

Calculator::Result Calculator::Multiply(double a, double b) {
    return pImpl_->ExecuteOperation(Operation::Multiply, a, b);
}

Calculator::Result Calculator::Divide(double a, double b) {
    return pImpl_->ExecuteOperation(Operation::Divide, a, b);
}

} // namespace ipc_demo
