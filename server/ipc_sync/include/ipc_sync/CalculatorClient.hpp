/**
 * @file Calculator.hpp
 * @brief Client-side proxy for Calculator service (Pimpl interface)
 */
#ifndef IPC_SYNC_CALCULATOR_HPP
#define IPC_SYNC_CALCULATOR_HPP

#include "ipc_sync/Channel.hpp"
#include <memory>
#include <string>

namespace ipc_demo {

/**
 * @class Calculator
 * @brief Client proxy for Calculator service
 * 
 * Uses Pimpl idiom - all implementation is hidden in shared library
 */
class Calculator {
public:
    struct Result {
        bool success;
        double value;
        std::string error_message;
    };

    /**
     * @brief Construct Calculator proxy
     * @param channel Communication channel
     */
    explicit Calculator(std::shared_ptr<Channel> channel);
    
    ~Calculator();

    // Disable copy/move
    Calculator(const Calculator&) = delete;
    Calculator& operator=(const Calculator&) = delete;
    Calculator(Calculator&&) = delete;
    Calculator& operator=(Calculator&&) = delete;

    /**
     * @brief Add two numbers
     */
    Result Add(double a, double b);

    /**
     * @brief Subtract two numbers
     */
    Result Subtract(double a, double b);

    /**
     * @brief Multiply two numbers
     */
    Result Multiply(double a, double b);

    /**
     * @brief Divide two numbers
     */
    Result Divide(double a, double b);

private:
    // Opaque pointer to implementation
    struct Impl;
    std::unique_ptr<Impl> pImpl_;
};

} // namespace ipc_demo

#endif // IPC_SYNC_CALCULATOR_HPP
