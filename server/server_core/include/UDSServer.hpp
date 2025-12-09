/**
 * @file UDSServer.hpp
 * @brief Unix Domain Socket server with epoll
 * 
 * Provides robust server implementation with:
 * - Epoll-based event loop
 * - Connection management
 * - Inactivity timeout
 * - Error recovery
 */

#ifndef IPC_DEMO_UDS_SERVER_HPP
#define IPC_DEMO_UDS_SERVER_HPP

#include "ServiceManager.hpp"
#include "ipc_sync/Protocol.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <memory>
#include <sys/epoll.h>

namespace ipc_demo {

/**
 * @struct ClientInfo
 * @brief Information about connected client
 */
struct ClientInfo {
    int fd;
    time_t last_activity;
    std::vector<uint8_t> recv_buffer;
};

/**
 * @class UDSServer
 * @brief Unix Domain Socket server
 * 
 * Single Responsibility: Manages network communication and event loop
 * Depends on ServiceManager abstraction (Dependency Inversion)
 */
class UDSServer {
public:
    /**
     * @brief Construct UDS server
     * @param socket_path Path to Unix socket file
     * @param service_manager Service registry
     */
    explicit UDSServer(const std::string& socket_path,
                       std::shared_ptr<ServiceManager> service_manager);

    ~UDSServer();

    // Disable copy/move
    UDSServer(const UDSServer&) = delete;
    UDSServer& operator=(const UDSServer&) = delete;
    UDSServer(UDSServer&&) = delete;
    UDSServer& operator=(UDSServer&&) = delete;

    /**
     * @brief Start the server in background thread
     * @return true if started successfully
     */
    bool Start();

    /**
     * @brief Stop the server gracefully
     */
    void Stop();

    /**
     * @brief Check if server is running
     * @return true if running
     */
    bool IsRunning() const { return running_.load(); }

    /**
     * @brief Get number of connected clients
     * @return Client count
     */
    size_t GetClientCount() const;

private:
    enum class ServerState {
        CreateSocket,
        ListenSocket,
        WaitAndHandleEvents,
        Cleanup,
        Exit
    };

    std::string socket_path_;
    std::shared_ptr<ServiceManager> service_manager_;
    
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    
    int server_fd_{-1};
    int epoll_fd_{-1};
    int timer_fd_{-1};
    
    std::unordered_map<int, std::unique_ptr<ClientInfo>> clients_;
    
    // Server thread main loop
    void ServerThreadFunc();
    
    // State machine handlers
    ServerState HandleCreateSocket();
    ServerState HandleListenSocket();
    ServerState HandleWaitAndHandleEvents();
    ServerState HandleCleanup();
    
    // Event handlers
    bool HandleNewConnection();
    bool HandleClientData(int client_fd);
    void HandleClientClose(int client_fd);
    void HandleInactivityTimer();
    
    // Protocol handling
    size_t ProcessClientRequest(int client_fd, const uint8_t* data, size_t len);
    
    // Utility methods
    bool CreateInactivityTimer();
    bool SetNonBlocking(int fd);
    void CloseAllClients();
    void RemoveSocketFile();
};

} // namespace ipc_demo

#endif // IPC_DEMO_UDS_SERVER_HPP
