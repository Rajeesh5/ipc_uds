/**
 * @file Channel.cpp
 * @brief Implementation of Channel (stays on server side, part of shared library)
 */

#include "ipc_sync/Channel.hpp"
#include "ipc_sync/ByteBuffer.hpp"
#include "ipc_sync/Protocol.hpp"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <mutex>
#include <atomic>
#include <vector>

namespace ipc_demo {

// Private implementation (hidden from client)
struct Channel::Impl {
    std::string socket_path_;
    int timeout_ms_;
    int socket_fd_;
    std::atomic<bool> connected_;
    std::string last_error_;
    std::mutex mutex_;
    std::vector<uint8_t> buffer_;

    Impl(const std::string& socket_path, int timeout_ms)
        : socket_path_(socket_path)
        , timeout_ms_(timeout_ms)
        , socket_fd_(-1)
        , connected_(false) {
        buffer_.resize(Protocol::MAX_PACKET_SIZE);
    }

    ~Impl() {
        Disconnect();
    }

    bool Connect() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (connected_.load() && socket_fd_ >= 0) {
            return true;
        }

        // Close existing connection if any
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        // Create socket
        socket_fd_ = socket(AF_UNIX, SOCK_STREAM, 0);
        if (socket_fd_ < 0) {
            last_error_ = "Failed to create socket: " + std::string(strerror(errno));
            return false;
        }

        // Set non-blocking for connect timeout
        int flags = fcntl(socket_fd_, F_GETFL, 0);
        fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK);

        // Setup server address
        struct sockaddr_un server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sun_family = AF_UNIX;
        strncpy(server_addr.sun_path, socket_path_.c_str(), sizeof(server_addr.sun_path) - 1);

        // Attempt connection
        int ret = connect(socket_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        if (ret < 0) {
            if (errno != EINPROGRESS) {
                last_error_ = "Connect failed: " + std::string(strerror(errno));
                close(socket_fd_);
                socket_fd_ = -1;
                return false;
            }

            // Wait for connection with timeout
            struct pollfd pfd;
            pfd.fd = socket_fd_;
            pfd.events = POLLOUT;

            ret = poll(&pfd, 1, timeout_ms_);
            if (ret <= 0) {
                last_error_ = ret == 0 ? "Connection timeout" : "Poll failed: " + std::string(strerror(errno));
                close(socket_fd_);
                socket_fd_ = -1;
                return false;
            }

            // Check if connection succeeded
            int error = 0;
            socklen_t len = sizeof(error);
            getsockopt(socket_fd_, SOL_SOCKET, SO_ERROR, &error, &len);
            if (error != 0) {
                last_error_ = "Connection failed: " + std::string(strerror(error));
                close(socket_fd_);
                socket_fd_ = -1;
                return false;
            }
        }

        // Restore blocking mode
        fcntl(socket_fd_, F_SETFL, flags);
        
        // Set socket timeouts
        struct timeval tv;
        tv.tv_sec = timeout_ms_ / 1000;
        tv.tv_usec = (timeout_ms_ % 1000) * 1000;
        setsockopt(socket_fd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        setsockopt(socket_fd_, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

        connected_.store(true);
        last_error_.clear();
        return true;
    }

    void Disconnect() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (socket_fd_ >= 0) {
            close(socket_fd_);
            socket_fd_ = -1;
        }
        connected_.store(false);
    }

    bool ExecuteRPC(uint32_t routine_id,
                    const uint8_t* request_data, size_t request_len,
                    uint8_t* response_buffer, size_t response_buffer_size,
                    size_t& response_len) {
        
        // Auto-reconnect if needed (handles timeouts transparently)
        if (!EnsureConnected()) {
            last_error_ = "Failed to establish connection";
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        // Build request frame
        ByteBuffer request_buf(buffer_.data(), buffer_.size());
        
        request_buf.PutByte(Protocol::START_BYTE);
        request_buf.PutInt(0); // Placeholder for length
        request_buf.PutInt(routine_id);
        request_buf.PutByte(Protocol::VERSION);
        
        // Add payload
        if (request_data && request_len > 0) {
            size_t pos = request_buf.Position();
            if (pos + request_len >= buffer_.size()) {
                last_error_ = "Request payload too large";
                return false;
            }
            std::memcpy(buffer_.data() + pos, request_data, request_len);
            request_buf.SetPosition(pos + request_len);
        }
        
        request_buf.PutByte(Protocol::END_BYTE);

        // Update frame length
        size_t frame_len = request_buf.Position();
        request_buf.SetPosition(1);
        request_buf.PutInt(static_cast<uint32_t>(frame_len));

        // Send request (with retry on connection failure)
        if (!SendData(buffer_.data(), frame_len)) {
            connected_.store(false);
            
            // Retry once after reconnecting
            if (Connect() && SendData(buffer_.data(), frame_len)) {
                // Successfully reconnected and sent
            } else {
                last_error_ = "Failed to send after reconnect attempt";
                return false;
            }
        }

        // Receive response
        if (!ReceiveData(response_buffer, response_buffer_size, response_len)) {
            connected_.store(false);
            last_error_ = "Failed to receive response";
            return false;
        }

        return true;
    }

    // Helper: Ensures connection is active (auto-reconnects if needed)
    bool EnsureConnected() {
        if (connected_.load() && socket_fd_ >= 0) {
            return true; // Already connected
        }
        
        // Need to (re)connect
        return Connect();
    }

    bool SendData(const uint8_t* data, size_t len) {
        size_t total_sent = 0;

        while (total_sent < len) {
            ssize_t sent = send(socket_fd_, data + total_sent, len - total_sent, MSG_NOSIGNAL);
            
            if (sent < 0) {
                if (errno == EINTR) {
                    continue;
                }
                last_error_ = "send failed: " + std::string(strerror(errno));
                return false;
            }

            if (sent == 0) {
                last_error_ = "Connection closed by server";
                return false;
            }

            total_sent += sent;
        }

        return true;
    }

    bool ReceiveData(uint8_t* data, size_t max_len, size_t& received) {
        received = 0;

        // Read minimum frame size
        size_t min_size = Protocol::GetMinFrameSize();
        while (received < min_size) {
            ssize_t n = recv(socket_fd_, data + received, max_len - received, 0);
            
            if (n < 0) {
                if (errno == EINTR) {
                    continue;
                }
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    last_error_ = "Receive timeout";
                    return false;
                }
                last_error_ = "recv failed: " + std::string(strerror(errno));
                return false;
            }

            if (n == 0) {
                last_error_ = "Connection closed by server";
                return false;
            }

            received += n;
        }

        // Parse frame length
        try {
            ByteBuffer buf(data, received);
            buf.GetByte(); // START_BYTE
            uint32_t frame_len = buf.GetInt();

            // Read remaining data if needed
            while (received < frame_len && received < max_len) {
                ssize_t n = recv(socket_fd_, data + received, max_len - received, 0);
                
                if (n < 0) {
                    if (errno == EINTR) {
                        continue;
                    }
                    last_error_ = "recv failed: " + std::string(strerror(errno));
                    return false;
                }

                if (n == 0) {
                    break;
                }

                received += n;
            }

            return true;

        } catch (const std::exception& e) {
            last_error_ = std::string("Error parsing response: ") + e.what();
            return false;
        }
    }
};

// Public interface implementation
Channel::Channel(const std::string& socket_path, int timeout_ms)
    : pImpl_(std::make_unique<Impl>(socket_path, timeout_ms)) {
    // Auto-connect on construction
    // Note: If initial connection fails, it will auto-retry on first API call
    pImpl_->Connect();
}

Channel::~Channel() {
    // Auto-disconnect on destruction (RAII)
    if (pImpl_) {
        pImpl_->Disconnect();
    }
}

bool Channel::Connect() {
    return pImpl_->Connect();
}

bool Channel::ExecuteRPC(uint32_t routine_id,
                         const uint8_t* request_data, size_t request_len,
                         uint8_t* response_buffer, size_t response_buffer_size,
                         size_t& response_len) {
    return pImpl_->ExecuteRPC(routine_id, request_data, request_len,
                              response_buffer, response_buffer_size, response_len);
}

bool Channel::IsConnected() const {
    return pImpl_->connected_.load();
}

std::string Channel::GetLastError() const {
    std::lock_guard<std::mutex> lock(pImpl_->mutex_);
    return pImpl_->last_error_;
}

void Channel::Disconnect() {
    pImpl_->Disconnect();
}

} // namespace ipc_demo
