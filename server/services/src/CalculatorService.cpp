/**
 * @file CalculatorService.cpp
 * @brief Implementation of Calculator service
 */

#include "CalculatorService.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"
#include <iostream>
#include <cmath>

namespace ipc_demo {

size_t CalculatorService::Execute(const uint8_t* input, size_t input_len,
                                  uint8_t* output, size_t output_len) {
    try {
        // Parse request
        ByteBuffer request(const_cast<uint8_t*>(input), input_len);
        
        uint8_t op_byte = request.GetByte();
        double operand_a = request.GetDouble();
        double operand_b = request.GetDouble();

        Operation op = static_cast<Operation>(op_byte);

        std::cout << "[CalculatorService] Request: op=" << (int)op_byte
                  << ", a=" << operand_a << ", b=" << operand_b << std::endl;

        // Execute operation
        double result = 0.0;
        std::string error_msg;
        Status status = ExecuteOperation(op, operand_a, operand_b, result, error_msg);

        // Build response frame
        ByteBuffer response(output, output_len);
        
        response.PutByte(Protocol::START_BYTE);
        response.PutInt(0); // Placeholder for length
        response.PutInt(GetResponseRoutineId());
        response.PutByte(Protocol::VERSION);
        response.PutByte(static_cast<uint8_t>(status));
        response.PutDouble(result);
        response.PutString(error_msg);
        response.PutByte(Protocol::END_BYTE);

        // Update length
        size_t total_len = response.Position();
        response.SetPosition(1);
        response.PutInt(total_len);

        std::cout << "[CalculatorService] Response: status=" << (int)status
                  << ", result=" << result << std::endl;

        return total_len;

    } catch (const std::exception& e) {
        std::cerr << "[CalculatorService] Exception: " << e.what() << std::endl;
        
        // Build error response
        ByteBuffer response(output, output_len);
        response.PutByte(Protocol::START_BYTE);
        response.PutInt(0);
        response.PutInt(GetResponseRoutineId());
        response.PutByte(Protocol::VERSION);
        response.PutByte(static_cast<uint8_t>(Status::InvalidInput));
        response.PutDouble(0.0);
        response.PutString(std::string("Exception: ") + e.what());
        response.PutByte(Protocol::END_BYTE);

        size_t total_len = response.Position();
        response.SetPosition(1);
        response.PutInt(total_len);

        return total_len;
    }
}

CalculatorService::Status CalculatorService::ExecuteOperation(
    Operation op, double a, double b,
    double& result, std::string& error) {
    
    error.clear();
    result = 0.0;

    switch (op) {
        case Operation::Add:
            result = a + b;
            return Status::Success;

        case Operation::Subtract:
            result = a - b;
            return Status::Success;

        case Operation::Multiply:
            result = a * b;
            return Status::Success;

        case Operation::Divide:
            if (std::abs(b) < 1e-10) {
                error = "Division by zero";
                return Status::DivisionByZero;
            }
            result = a / b;
            return Status::Success;

        default:
            error = "Invalid operation code";
            return Status::InvalidOperation;
    }
}

} // namespace ipc_demo
