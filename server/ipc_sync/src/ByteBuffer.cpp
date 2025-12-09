/**
 * @file ByteBuffer.cpp
 * @brief Implementation of ByteBuffer serialization
 */

#include "ipc_sync/ByteBuffer.hpp"
#include <cstring>
#include <stdexcept>
#include <arpa/inet.h> // For htonl, ntohl (network byte order)

namespace ipc_demo {

ByteBuffer::ByteBuffer(uint8_t* buffer, size_t length)
    : buffer_(buffer), length_(length), position_(0) {
    if (buffer == nullptr) {
        throw std::invalid_argument("ByteBuffer: buffer cannot be null");
    }
    if (length == 0) {
        throw std::invalid_argument("ByteBuffer: length must be > 0");
    }
}

bool ByteBuffer::CheckBoundary(size_t size) const {
    return (position_ + size) <= length_;
}

void ByteBuffer::WriteBytes(const void* data, size_t size) {
    if (!CheckBoundary(size)) {
        throw std::overflow_error("ByteBuffer: write would exceed buffer bounds");
    }
    std::memcpy(buffer_ + position_, data, size);
    position_ += size;
}

void ByteBuffer::ReadBytes(void* data, size_t size) {
    if (!CheckBoundary(size)) {
        throw std::underflow_error("ByteBuffer: read would exceed buffer bounds");
    }
    std::memcpy(data, buffer_ + position_, size);
    position_ += size;
}

// Write operations
void ByteBuffer::PutByte(uint8_t data) {
    WriteBytes(&data, sizeof(data));
}

void ByteBuffer::PutInt(uint32_t data) {
    // Convert to network byte order for portability
    uint32_t net_data = htonl(data);
    WriteBytes(&net_data, sizeof(net_data));
}

void ByteBuffer::PutShort(uint16_t data) {
    uint16_t net_data = htons(data);
    WriteBytes(&net_data, sizeof(net_data));
}

void ByteBuffer::PutLong(int64_t data) {
    // Convert to network byte order (big-endian) for 64-bit value
    // Split into two 32-bit parts for portability
    uint32_t high = htonl(static_cast<uint32_t>(data >> 32));
    uint32_t low = htonl(static_cast<uint32_t>(data & 0xFFFFFFFF));
    WriteBytes(&high, sizeof(high));
    WriteBytes(&low, sizeof(low));
}

void ByteBuffer::PutFloat(float data) {
    // Note: This assumes IEEE 754 format
    WriteBytes(&data, sizeof(data));
}

void ByteBuffer::PutDouble(double data) {
    WriteBytes(&data, sizeof(data));
}

void ByteBuffer::PutString(const std::string& data) {
    // Format: [length:uint32_t][string data]
    uint32_t len = static_cast<uint32_t>(data.length());
    PutInt(len);
    if (len > 0) {
        WriteBytes(data.data(), len);
    }
}

void ByteBuffer::PutMap(const std::unordered_map<std::string, std::string>& data) {
    // Format: [count:uint32_t][key1:string][value1:string]...
    PutInt(static_cast<uint32_t>(data.size()));
    for (const auto& [key, value] : data) {
        PutString(key);
        PutString(value);
    }
}

void ByteBuffer::PutArray(const uint8_t* data, uint32_t len) {
    // Format: [length:uint32_t][data bytes]
    PutInt(len);
    if (len > 0) {
        if (data == nullptr) {
            throw std::invalid_argument("ByteBuffer::PutArray: data cannot be null");
        }
        WriteBytes(data, len);
    }
}

// Read operations
uint8_t ByteBuffer::GetByte() {
    uint8_t data = 0;
    ReadBytes(&data, sizeof(data));
    return data;
}

uint32_t ByteBuffer::GetInt() {
    uint32_t net_data = 0;
    ReadBytes(&net_data, sizeof(net_data));
    return ntohl(net_data); // Convert from network byte order
}

uint16_t ByteBuffer::GetShort() {
    uint16_t net_data = 0;
    ReadBytes(&net_data, sizeof(net_data));
    return ntohs(net_data);
}

int64_t ByteBuffer::GetLong() {
    // Read two 32-bit parts and convert from network byte order
    uint32_t high = 0;
    uint32_t low = 0;
    ReadBytes(&high, sizeof(high));
    ReadBytes(&low, sizeof(low));
    high = ntohl(high);
    low = ntohl(low);
    return (static_cast<int64_t>(high) << 32) | low;
}

float ByteBuffer::GetFloat() {
    float data = 0.0f;
    ReadBytes(&data, sizeof(data));
    return data;
}

double ByteBuffer::GetDouble() {
    double data = 0.0;
    ReadBytes(&data, sizeof(data));
    return data;
}

std::string ByteBuffer::GetString() {
    uint32_t len = GetInt();
    if (len == 0) {
        return "";
    }
    
    if (!CheckBoundary(len)) {
        throw std::underflow_error("ByteBuffer::GetString: string length exceeds buffer");
    }
    
    std::string result(reinterpret_cast<char*>(buffer_ + position_), len);
    position_ += len;
    return result;
}

std::unordered_map<std::string, std::string> ByteBuffer::GetMap() {
    std::unordered_map<std::string, std::string> result;
    uint32_t count = GetInt();
    
    result.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        std::string key = GetString();
        std::string value = GetString();
        result[key] = value;
    }
    
    return result;
}

size_t ByteBuffer::GetArray(uint8_t* data, uint32_t max_len) {
    uint32_t len = GetInt();
    
    if (data == nullptr && len > 0) {
        throw std::invalid_argument("ByteBuffer::GetArray: data cannot be null");
    }
    
    if (len > max_len) {
        throw std::overflow_error("ByteBuffer::GetArray: array size exceeds destination buffer");
    }
    
    if (len > 0) {
        ReadBytes(data, len);
    }
    
    return len;
}

// Buffer management
void ByteBuffer::Reset() {
    position_ = 0;
}

void ByteBuffer::SetPosition(size_t pos) {
    if (pos > length_) {
        throw std::out_of_range("ByteBuffer::SetPosition: position exceeds buffer length");
    }
    position_ = pos;
}

} // namespace ipc_demo
