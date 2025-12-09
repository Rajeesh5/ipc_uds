/**
 * @file main.cpp
 * @brief Demo client application
 * 
 * Demonstrates how to use the IPC framework to call remote services
 * 
 * NOTE: This client has ZERO .cpp implementation files!
 * All implementations are in shared libraries from the server:
 *   - libipc_sync.so (UNIFIED CLIENT SDK)
 * 
 * TRUE CLIENT-SERVER SEPARATION using Pimpl pattern!
 */

#include "ipc_sync/Channel.hpp"
#include "ipc_sync/CalculatorClient.hpp"
#include "ipc_sync/TimeClient.hpp"
#include "ipc_sync/Protocol.hpp"
#include <iostream>
#include <iomanip>
 
using namespace ipc_demo;

void PrintResult(const std::string& operation, const Calculator::Result& result) {
    std::cout << operation << ": ";
    if (result.success) {
        std::cout << "Success! Result = " << std::fixed << std::setprecision(2) 
                  << result.value << std::endl;
    } else {
        std::cout << "Failed! Error: " << result.error_message << std::endl;
    }
}

int main() {
    std::cout << "=== IPC Demo Client ===" << std::endl;
    std::cout << "Connecting to: " << Protocol::UDS_PATH << "\n" << std::endl;

    try {
        // Create communication channel (auto-connects in constructor)
        auto channel = std::make_shared<Channel>(Protocol::UDS_PATH);
        
        // No need to call Connect() - constructor already did it!
        // If server is down, it will auto-retry on first API call
        std::cout << "[Client] Channel created (auto-connected)\n" << std::endl;

        // Create Calculator proxy
        auto calculator = std::make_unique<Calculator>(channel);

        // Demonstrate all operations
        std::cout << "=== Calculator Operations ===" << std::endl;

        // Addition
        auto result = calculator->Add(10.5, 5.3);
        PrintResult("10.5 + 5.3", result);

        // Subtraction
        result = calculator->Subtract(20.0, 8.5);
        PrintResult("20.0 - 8.5", result);

        // Multiplication
        result = calculator->Multiply(7.5, 4.0);
        PrintResult("7.5 * 4.0", result);

        // Division
        result = calculator->Divide(100.0, 5.0);
        PrintResult("100.0 / 5.0", result);

        // Division by zero (should fail gracefully)
        std::cout << "\n=== Error Handling Test ===" << std::endl;
        result = calculator->Divide(42.0, 0.0);
        PrintResult("42.0 / 0.0", result);

        // More complex calculations
        std::cout << "\n=== Complex Calculations ===" << std::endl;
        result = calculator->Add(-15.5, 20.3);
        PrintResult("-15.5 + 20.3", result);

        result = calculator->Multiply(0.5, 0.5);
        PrintResult("0.5 * 0.5", result);

        result = calculator->Divide(1.0, 3.0);
        PrintResult("1.0 / 3.0", result);

        // Test Time Service
        std::cout << "\n=== Time Service ===" << std::endl;
        auto timeClient = std::make_unique<TimeClient>(channel);
        
        auto timeResult = timeClient->GetCurrentTime();
        if (timeResult.success) {
            std::cout << "Server Time: " << timeResult.timestamp << std::endl;
            std::cout << "Unix Timestamp: " << timeResult.unix_timestamp << " seconds since epoch" << std::endl;
        } else {
            std::cout << "Failed to get time: " << timeResult.error_message << std::endl;
        }

        std::cout << "\n[Client] All operations completed!" << std::endl;
        
        // No need to call Disconnect() - destructor will handle it automatically!
        std::cout << "[Client] Done (auto-disconnecting via RAII)!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[Client] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
