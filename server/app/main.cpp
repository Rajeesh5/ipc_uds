/**
 * @file main.cpp
 * @brief IPC Demo Server Application
 * 
 * Initializes services and starts the UDS server
 */

#include "UDSServer.hpp"
#include "ServiceManager.hpp"
#include "CalculatorService.hpp"
#include "TimeService.hpp"
#include "ipc_sync/Protocol.hpp"
#include <iostream>
#include <csignal>
#include <memory>
#include <atomic>

using namespace ipc_demo;

std::atomic<bool> shutdown_requested{false};

void SignalHandler(int signal) {
    std::cout << "\n[Server] Received signal " << signal << ", shutting down..." << std::endl;
    shutdown_requested.store(true);
}

int main(int argc, char* argv[]) {
    std::cout << "=== IPC Demo Server ===" << std::endl;
    std::cout << "Socket path: " << Protocol::UDS_PATH << std::endl;
    std::cout << "Press Ctrl+C to stop\n" << std::endl;

    // Setup signal handlers
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);

    try {
        // Create service manager
        auto service_manager = std::make_shared<ServiceManager>();

        // Register services
        std::cout << "[Server] Registering services..." << std::endl;
        
        auto calculator_service = std::make_shared<CalculatorService>();
        if (!service_manager->RegisterService(calculator_service)) {
            std::cerr << "[Server] Failed to register CalculatorService" << std::endl;
            return 1;
        }

        auto time_service = std::make_shared<TimeService>();
        if (!service_manager->RegisterService(time_service)) {
            std::cerr << "[Server] Failed to register TimeService" << std::endl;
            return 1;
        }

        std::cout << "[Server] " << service_manager->GetServiceCount() 
                  << " service(s) registered\n" << std::endl;

        // Create and start server
        auto server = std::make_unique<UDSServer>(Protocol::UDS_PATH, service_manager);
        
        if (!server->Start()) {
            std::cerr << "[Server] Failed to start server" << std::endl;
            return 1;
        }

        std::cout << "[Server] Server is running...\n" << std::endl;

        // Main loop - wait for shutdown
        while (!shutdown_requested.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Graceful shutdown
        std::cout << "\n[Server] Shutting down gracefully..." << std::endl;
        server->Stop();
        service_manager->Clear();

        std::cout << "[Server] Shutdown complete" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[Server] Fatal error: " << e.what() << std::endl;
        return 1;
    }
}
