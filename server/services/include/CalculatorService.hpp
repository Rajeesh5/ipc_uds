/**
 * @file CalculatorService.hpp
 * @brief Calculator RPC service implementation
 * 
 * Demonstrates how to implement a service following SOLID principles
 */

#ifndef IPC_DEMO_CALCULATOR_SERVICE_HPP
#define IPC_DEMO_CALCULATOR_SERVICE_HPP

#include "IService.hpp"

namespace ipc_demo {

/**
 * @class CalculatorService
 * @brief Provides basic arithmetic operations via RPC
 * 
 * Operations:
 * - Add: a + b
 * - Subtract: a - b
 * - Multiply: a * b
 * - Divide: a / b (with error handling for division by zero)
 * 
 * Request Format (after version byte):
 *   [operation:byte][operand_a:double][operand_b:double]
 * 
 * Response Format (full frame):
 *   [START][LENGTH][RESPONSE_ID][VERSION][status:byte][result:double][error_msg:string][END]
 */
class CalculatorService : public IService {
public:
    // Operation codes
    enum class Operation : uint8_t {
        Add = 0x01,
        Subtract = 0x02,
        Multiply = 0x03,
        Divide = 0x04
    };

    // Status codes
    enum class Status : uint8_t {
        Success = 0x00,
        DivisionByZero = 0x01,
        InvalidOperation = 0x02,
        InvalidInput = 0x03
    };

    CalculatorService() = default;
    ~CalculatorService() override = default;

    uint32_t GetRequestRoutineId() const override {
        return 0x1000; // Calculator request ID
    }

    uint32_t GetResponseRoutineId() const override {
        return 0x1001; // Calculator response ID
    }

    size_t Execute(const uint8_t* input, size_t input_len,
                  uint8_t* output, size_t output_len) override;

    std::string GetName() const override {
        return "CalculatorService";
    }

private:
    /**
     * @brief Execute arithmetic operation
     * @param op Operation code
     * @param a First operand
     * @param b Second operand
     * @param result Output result
     * @param error Output error message
     * @return Status code
     */
    Status ExecuteOperation(Operation op, double a, double b,
                           double& result, std::string& error);
};

} // namespace ipc_demo

#endif // IPC_DEMO_CALCULATOR_SERVICE_HPP
