/**
 * @file ByteBuffer.hpp
 * @brief Interface for byte buffer serialization/deserialization
 * 
 * Provides methods to serialize and deserialize various data types
 * for network communication. Follows Interface Segregation Principle.
 */

#ifndef IPC_SYNC_BYTE_BUFFER_HPP
#define IPC_SYNC_BYTE_BUFFER_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

namespace ipc_demo {

/**
 * @class IByteBuffer
 * @brief Abstract interface for byte buffer operations
 * 
 * Allows serialization/deserialization of primitive types, strings,
 * maps, and arrays for network communication.
 */
class IByteBuffer {
public:
    virtual ~IByteBuffer() = default;

    // Write operations
    virtual void PutByte(uint8_t data) = 0;
    virtual void PutInt(uint32_t data) = 0;
    virtual void PutShort(uint16_t data) = 0;
    virtual void PutLong(int64_t data) = 0;
    virtual void PutFloat(float data) = 0;
    virtual void PutDouble(double data) = 0;
    virtual void PutString(const std::string& data) = 0;
    virtual void PutMap(const std::unordered_map<std::string, std::string>& data) = 0;
    virtual void PutArray(const uint8_t* data, uint32_t len) = 0;

    // Read operations
    virtual uint8_t GetByte() = 0;
    virtual uint32_t GetInt() = 0;
    virtual uint16_t GetShort() = 0;
    virtual int64_t GetLong() = 0;
    virtual float GetFloat() = 0;
    virtual double GetDouble() = 0;
    virtual std::string GetString() = 0;
    virtual std::unordered_map<std::string, std::string> GetMap() = 0;
    virtual size_t GetArray(uint8_t* data, uint32_t max_len) = 0;

    // Buffer management
    virtual void Reset() = 0;
    virtual size_t Position() const = 0;
    virtual void SetPosition(size_t pos) = 0;
    virtual size_t Capacity() const = 0;
};

/**
 * @class ByteBuffer
 * @brief Concrete implementation of IByteBuffer
 * 
 * Handles byte-level serialization with boundary checking
 * and proper error handling. Single Responsibility: serialization only.
 */
class ByteBuffer : public IByteBuffer {
public:
    /**
     * @brief Construct ByteBuffer with external buffer
     * @param buffer Pointer to buffer (not owned)
     * @param length Buffer capacity in bytes
     */
    ByteBuffer(uint8_t* buffer, size_t length);

    // Disable copy/move to prevent buffer ownership issues
    ByteBuffer(const ByteBuffer&) = delete;
    ByteBuffer& operator=(const ByteBuffer&) = delete;
    ByteBuffer(ByteBuffer&&) = delete;
    ByteBuffer& operator=(ByteBuffer&&) = delete;

    ~ByteBuffer() override = default;

    // Write operations
    void PutByte(uint8_t data) override;
    void PutInt(uint32_t data) override;
    void PutShort(uint16_t data) override;
    void PutLong(int64_t data) override;
    void PutFloat(float data) override;
    void PutDouble(double data) override;
    void PutString(const std::string& data) override;
    void PutMap(const std::unordered_map<std::string, std::string>& data) override;
    void PutArray(const uint8_t* data, uint32_t len) override;

    // Read operations
    uint8_t GetByte() override;
    uint32_t GetInt() override;
    uint16_t GetShort() override;
    int64_t GetLong() override;
    float GetFloat() override;
    double GetDouble() override;
    std::string GetString() override;
    std::unordered_map<std::string, std::string> GetMap() override;
    size_t GetArray(uint8_t* data, uint32_t max_len) override;

    // Buffer management
    void Reset() override;
    size_t Position() const override { return position_; }
    void SetPosition(size_t pos) override;
    size_t Capacity() const override { return length_; }

private:
    uint8_t* buffer_;         // External buffer (not owned)
    const size_t length_;     // Buffer capacity
    size_t position_;         // Current read/write position

    /**
     * @brief Check if operation would exceed buffer bounds
     * @param size Number of bytes to check
     * @return true if operation is safe, false otherwise
     */
    bool CheckBoundary(size_t size) const;

    /**
     * @brief Write raw bytes to buffer
     * @param data Source data
     * @param size Number of bytes
     */
    void WriteBytes(const void* data, size_t size);

    /**
     * @brief Read raw bytes from buffer
     * @param data Destination buffer
     * @param size Number of bytes
     */
    void ReadBytes(void* data, size_t size);
};

/**
 * @brief Factory function to create ByteBuffer instances
 * @param buffer External buffer pointer
 * @param length Buffer capacity
 * @return Unique pointer to ByteBuffer
 */
inline std::unique_ptr<IByteBuffer> CreateByteBuffer(uint8_t* buffer, size_t length) {
    return std::make_unique<ByteBuffer>(buffer, length);
}

} // namespace ipc_demo

#endif // IPC_SYNC_BYTE_BUFFER_HPP
