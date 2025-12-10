/**
 * @file UDSServer.cpp
 * @brief Implementation of UDS server
 */

#include "UDSServer.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace ipc_demo {

UDSServer::UDSServer(const std::string& socket_path,
                     std::shared_ptr<ServiceManager> service_manager)
    : socket_path_(socket_path)
    , service_manager_(service_manager) {
    
    if (!service_manager_) {
        throw std::invalid_argument("UDSServer: service_manager cannot be null");
    }
}

UDSServer::~UDSServer() {
    Stop();
}

bool UDSServer::Start() {
    if (running_.load()) {
        std::cerr << "[UDSServer] Already running" << std::endl;
        return false;
    }

    running_.store(true);
    server_thread_ = std::thread(&UDSServer::ServerThreadFunc, this);
    
    std::cout << "[UDSServer] Started on: " << socket_path_ << std::endl;
    return true;
}

void UDSServer::Stop() {
    if (!running_.load()) {
        return;
    }

    std::cout << "[UDSServer] Stopping..." << std::endl;
    running_.store(false);

    if (server_thread_.joinable()) {
        server_thread_.join();
    }

    std::cout << "[UDSServer] Stopped" << std::endl;
}

size_t UDSServer::GetClientCount() const {
    return clients_.size();
}

void UDSServer::ServerThreadFunc() {
    ServerState state = ServerState::CreateSocket;

    while (running_.load() && state != ServerState::Exit) {
        switch (state) {
            case ServerState::CreateSocket:
                state = HandleCreateSocket();
                break;
            case ServerState::ListenSocket:
                state = HandleListenSocket();
                break;
            case ServerState::WaitAndHandleEvents:
                state = HandleWaitAndHandleEvents();
                break;
            case ServerState::Cleanup:
                state = HandleCleanup();
                break;
            default:
                state = ServerState::Exit;
                break;
        }
    }

    HandleCleanup();
}

UDSServer::ServerState UDSServer::HandleCreateSocket() {
    // Remove existing socket file
    RemoveSocketFile();

    // Create socket
    server_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        std::cerr << "[UDSServer] Failed to create socket: " << strerror(errno) << std::endl;
        return ServerState::Exit;
    }

    // Set non-blocking
    if (!SetNonBlocking(server_fd_)) {
        std::cerr << "[UDSServer] Failed to set non-blocking" << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return ServerState::Exit;
    }

    // Bind to path
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path_.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[UDSServer] Bind failed: " << strerror(errno) << std::endl;
        close(server_fd_);
        server_fd_ = -1;
        return ServerState::Exit;
    }

    std::cout << "[UDSServer] Socket created and bound" << std::endl;
    return ServerState::ListenSocket;
}

UDSServer::ServerState UDSServer::HandleListenSocket() {
    if (listen(server_fd_, 10) < 0) {
        std::cerr << "[UDSServer] Listen failed: " << strerror(errno) << std::endl;
        return ServerState::Cleanup;
    }

    // Create epoll instance
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        std::cerr << "[UDSServer] epoll_create1 failed: " << strerror(errno) << std::endl;
        return ServerState::Cleanup;
    }

    // Add server socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) < 0) {
        std::cerr << "[UDSServer] epoll_ctl failed for server_fd: " << strerror(errno) << std::endl;
        return ServerState::Cleanup;
    }

    // Create inactivity timer
    if (!CreateInactivityTimer()) {
        std::cerr << "[UDSServer] Failed to create inactivity timer" << std::endl;
        return ServerState::Cleanup;
    }

    std::cout << "[UDSServer] Listening for connections" << std::endl;
    return ServerState::WaitAndHandleEvents;
}

UDSServer::ServerState UDSServer::HandleWaitAndHandleEvents() {
    constexpr int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];

    int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000); // 1 second timeout
    
    if (nfds < 0) {
        if (errno == EINTR) {
            return ServerState::WaitAndHandleEvents; // Interrupted, continue
        }
        std::cerr << "[UDSServer] epoll_wait failed: " << strerror(errno) << std::endl;
        return ServerState::Cleanup;
    }

    for (int i = 0; i < nfds; ++i) {
        int fd = events[i].data.fd;

        if (fd == server_fd_) {
            // New connection
            if (!HandleNewConnection()) {
                std::cerr << "[UDSServer] Failed to accept new connection" << std::endl;
            }
        } else if (fd == timer_fd_) {
            // Inactivity timer fired
            HandleInactivityTimer();
        } else {
            // Client data or error
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                HandleClientClose(fd);
            } else if (events[i].events & EPOLLIN) {
                if (!HandleClientData(fd)) {
                    HandleClientClose(fd);
                }
            }
        }
    }

    return ServerState::WaitAndHandleEvents;
}

UDSServer::ServerState UDSServer::HandleCleanup() {
    std::cout << "[UDSServer] Cleaning up..." << std::endl;

    CloseAllClients();

    if (timer_fd_ >= 0) {
        close(timer_fd_);
        timer_fd_ = -1;
    }

    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }

    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }

    RemoveSocketFile();

    return ServerState::Exit;
}

bool UDSServer::HandleNewConnection() {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    if (client_fd < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "[UDSServer] Accept failed: " << strerror(errno) << std::endl;
        }
        return false;
    }

    if (!SetNonBlocking(client_fd)) {
        std::cerr << "[UDSServer] Failed to set client non-blocking" << std::endl;
        close(client_fd);
        return false;
    }

    // Add to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // Edge-triggered
    ev.data.fd = client_fd;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
        std::cerr << "[UDSServer] epoll_ctl failed for client: " << strerror(errno) << std::endl;
        close(client_fd);
        return false;
    }

    // Create client info
    auto client = std::make_unique<ClientInfo>();
    client->fd = client_fd;
    client->last_activity = time(nullptr);
    client->recv_buffer.reserve(Protocol::MAX_PACKET_SIZE);

    clients_[client_fd] = std::move(client);

    std::cout << "[UDSServer] New client connected (fd=" << client_fd 
              << ", total=" << clients_.size() << ")" << std::endl;

    return true;
}

bool UDSServer::HandleClientData(int client_fd) {
    auto it = clients_.find(client_fd);
    if (it == clients_.end()) {
        return false;
    }

    auto& client = it->second;
    uint8_t buffer[Protocol::MAX_PACKET_SIZE];

    // Read available data
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer), 0);
    
    if (bytes_read < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "[UDSServer] recv failed: " << strerror(errno) << std::endl;
            return false;
        }
        return true; // No data available
    }

    if (bytes_read == 0) {
        // Connection closed by client
        std::cout << "[UDSServer] Client disconnected (fd=" << client_fd << ")" << std::endl;
        return false;
    }

    // Update activity timestamp
    client->last_activity = time(nullptr);

    // Process the request
    size_t response_len = ProcessClientRequest(client_fd, buffer, bytes_read);
    (void)response_len; // Response sent directly to client in ProcessClientRequest
    
    return true; // Keep connection alive
}

void UDSServer::HandleClientClose(int client_fd) {
    auto it = clients_.find(client_fd);
    if (it == clients_.end()) {
        return;
    }

    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
    clients_.erase(it);

    std::cout << "[UDSServer] Client closed (fd=" << client_fd 
              << ", remaining=" << clients_.size() << ")" << std::endl;
}

void UDSServer::HandleInactivityTimer() {
    uint64_t expirations;
    read(timer_fd_, &expirations, sizeof(expirations));

    time_t now = time(nullptr);
    std::vector<int> inactive_clients;

    for (const auto& [fd, client] : clients_) {
        if ((now - client->last_activity) > Protocol::INACTIVITY_TIMEOUT_SEC) {
            inactive_clients.push_back(fd);
        }
    }

    for (int fd : inactive_clients) {
        std::cout << "[UDSServer] Closing inactive client (fd=" << fd << ")" << std::endl;
        HandleClientClose(fd);
    }
}

size_t UDSServer::ProcessClientRequest(int client_fd, const uint8_t* data, size_t len) {
    if (len < Protocol::GetMinFrameSize()) {
        std::cerr << "[UDSServer] Packet too small: " << len << " bytes" << std::endl;
        return 0;
    }

    try {
        // Parse frame
        ByteBuffer request(const_cast<uint8_t*>(data), len);
        
        uint8_t start = request.GetByte();
        if (start != Protocol::START_BYTE) {
            std::cerr << "[UDSServer] Invalid start byte: 0x" << std::hex << (int)start << std::dec << std::endl;
            return 0;
        }

        uint32_t frame_len = request.GetInt();
        (void)frame_len; // Frame length already validated in protocol parsing
        
        uint32_t routine_id = request.GetInt();
        uint8_t version = request.GetByte();

        if (version != Protocol::VERSION) {
            std::cerr << "[UDSServer] Unsupported version: " << (int)version << std::endl;
            return 0;
        }

        // Prepare response buffer
        uint8_t response[Protocol::MAX_PACKET_SIZE];
        
        // Execute service
        size_t payload_start = request.Position();
        size_t payload_len = len - payload_start - 1; // Exclude END_BYTE
        
        size_t response_len = service_manager_->ExecuteService(
            routine_id,
            data + payload_start,
            payload_len,
            response,
            sizeof(response)
        );

        if (response_len > 0) {
            // Send response
            ssize_t sent = send(client_fd, response, response_len, MSG_NOSIGNAL);
            if (sent < 0) {
                std::cerr << "[UDSServer] send failed: " << strerror(errno) << std::endl;
                return 0;
            }
            return response_len;
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[UDSServer] Exception processing request: " << e.what() << std::endl;
        return 0;
    }
}

bool UDSServer::CreateInactivityTimer() {
    timer_fd_ = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer_fd_ < 0) {
        std::cerr << "[UDSServer] timerfd_create failed: " << strerror(errno) << std::endl;
        return false;
    }

    // Set timer to fire every 60 seconds
    struct itimerspec its;
    its.it_value.tv_sec = 60;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 60;
    its.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd_, 0, &its, nullptr) < 0) {
        std::cerr << "[UDSServer] timerfd_settime failed: " << strerror(errno) << std::endl;
        close(timer_fd_);
        timer_fd_ = -1;
        return false;
    }

    // Add to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = timer_fd_;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, timer_fd_, &ev) < 0) {
        std::cerr << "[UDSServer] epoll_ctl failed for timer: " << strerror(errno) << std::endl;
        close(timer_fd_);
        timer_fd_ = -1;
        return false;
    }

    return true;
}

bool UDSServer::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) >= 0;
}

void UDSServer::CloseAllClients() {
    std::cout << "[UDSServer] Closing " << clients_.size() << " clients" << std::endl;
    
    for (auto& [fd, client] : clients_) {
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
        close(fd);
    }
    
    clients_.clear();
}

void UDSServer::RemoveSocketFile() {
    if (unlink(socket_path_.c_str()) < 0 && errno != ENOENT) {
        std::cerr << "[UDSServer] Warning: Failed to remove socket file: " 
                  << strerror(errno) << std::endl;
    }
}

} // namespace ipc_demo
