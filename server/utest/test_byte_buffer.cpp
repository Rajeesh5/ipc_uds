/**
 * @file test_byte_buffer.cpp
 * @brief Unit tests for ByteBuffer serialization
 */

#include "ipc_sync/ByteBuffer.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <cstring>

using namespace ipc_demo;

class ByteBufferTest : public ::testing::Test {
protected:
    std::vector<uint8_t> buffer_;
    
    void SetUp() override {
        buffer_.resize(1024);
    }
};

TEST_F(ByteBufferTest, WriteAndReadByte) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutByte(0x42);
    buf.PutByte(0xFF);
    buf.PutByte(0x00);
    
    buf.Reset();
    EXPECT_EQ(buf.GetByte(), 0x42);
    EXPECT_EQ(buf.GetByte(), 0xFF);
    EXPECT_EQ(buf.GetByte(), 0x00);
}

TEST_F(ByteBufferTest, WriteAndReadInt) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutInt(0x12345678);
    buf.PutInt(0xABCDEF00);
    buf.PutInt(0);
    buf.PutInt(0xFFFFFFFF);
    
    buf.Reset();
    EXPECT_EQ(buf.GetInt(), 0x12345678u);
    EXPECT_EQ(buf.GetInt(), 0xABCDEF00u);
    EXPECT_EQ(buf.GetInt(), 0u);
    EXPECT_EQ(buf.GetInt(), 0xFFFFFFFFu);
}

TEST_F(ByteBufferTest, WriteAndReadDouble) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutDouble(3.14159);
    buf.PutDouble(-2.71828);
    buf.PutDouble(0.0);
    buf.PutDouble(1e100);
    
    buf.Reset();
    EXPECT_DOUBLE_EQ(buf.GetDouble(), 3.14159);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), -2.71828);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), 0.0);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), 1e100);
}

TEST_F(ByteBufferTest, WriteAndReadString) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    std::string str1 = "Hello, World!";
    std::string str2 = "";
    std::string str3 = "Special chars: 中文 русский 🎉";
    std::string str4 = "Line1\nLine2\tTabbed";
    
    buf.PutString(str1);
    buf.PutString(str2);
    buf.PutString(str3);
    buf.PutString(str4);
    
    buf.Reset();
    EXPECT_EQ(buf.GetString(), str1);
    EXPECT_EQ(buf.GetString(), str2);
    EXPECT_EQ(buf.GetString(), str3);
    EXPECT_EQ(buf.GetString(), str4);
}

TEST_F(ByteBufferTest, WriteAndReadMap) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    std::unordered_map<std::string, std::string> map;
    map["key1"] = "value1";
    map["key2"] = "value2";
    map["empty"] = "";
    map["special"] = "中文";
    
    buf.PutMap(map);
    
    buf.Reset();
    auto result = buf.GetMap();
    
    EXPECT_EQ(result.size(), map.size());
    EXPECT_EQ(result["key1"], "value1");
    EXPECT_EQ(result["key2"], "value2");
    EXPECT_EQ(result["empty"], "");
    EXPECT_EQ(result["special"], "中文");
}

TEST_F(ByteBufferTest, EmptyMap) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    std::unordered_map<std::string, std::string> empty_map;
    buf.PutMap(empty_map);
    
    buf.Reset();
    auto result = buf.GetMap();
    
    EXPECT_TRUE(result.empty());
}

TEST_F(ByteBufferTest, BufferOverflow) {
    std::vector<uint8_t> small_buffer(10);
    ByteBuffer buf(small_buffer.data(), small_buffer.size());
    
    EXPECT_THROW(buf.PutString("This string is way too long for the buffer"), std::overflow_error);
}

TEST_F(ByteBufferTest, BufferUnderflow) {
    // Use a small buffer that will actually overflow
    std::vector<uint8_t> small_buf(8); // Only room for 2 ints
    ByteBuffer buf(small_buf.data(), small_buf.size());
    
    buf.PutInt(42);
    buf.PutInt(100);
    
    buf.Reset();
    EXPECT_EQ(buf.GetInt(), 42u);   // OK
    EXPECT_EQ(buf.GetInt(), 100u);  // OK
    
    // Now try to read a third int - should throw
    EXPECT_THROW(buf.GetInt(), std::underflow_error);
}

TEST_F(ByteBufferTest, PositionManagement) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutInt(100);
    EXPECT_EQ(buf.Position(), 4u);
    
    buf.PutByte(0xFF);
    EXPECT_EQ(buf.Position(), 5u);
    
    buf.SetPosition(0);
    EXPECT_EQ(buf.Position(), 0u);
    EXPECT_EQ(buf.GetInt(), 100u);
    
    buf.Reset();
    EXPECT_EQ(buf.Position(), 0u);
}

TEST_F(ByteBufferTest, MixedDataTypes) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    // Write mixed data
    buf.PutByte(0x42);
    buf.PutInt(0x12345678);
    buf.PutDouble(3.14159);
    buf.PutString("test");
    
    // Read back in order
    buf.Reset();
    EXPECT_EQ(buf.GetByte(), 0x42);
    EXPECT_EQ(buf.GetInt(), 0x12345678u);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), 3.14159);
    EXPECT_EQ(buf.GetString(), "test");
}

TEST_F(ByteBufferTest, LargeString) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    std::string large_string(500, 'X');
    buf.PutString(large_string);
    
    buf.Reset();
    EXPECT_EQ(buf.GetString(), large_string);
}

TEST_F(ByteBufferTest, ZeroValues) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutByte(0);
    buf.PutInt(0);
    buf.PutDouble(0.0);
    
    buf.Reset();
    EXPECT_EQ(buf.GetByte(), 0);
    EXPECT_EQ(buf.GetInt(), 0u);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), 0.0);
}

TEST_F(ByteBufferTest, NegativeNumbers) {
    ByteBuffer buf(buffer_.data(), buffer_.size());
    
    buf.PutDouble(-123.456);
    buf.PutDouble(-1e-10);
    
    buf.Reset();
    EXPECT_DOUBLE_EQ(buf.GetDouble(), -123.456);
    EXPECT_DOUBLE_EQ(buf.GetDouble(), -1e-10);
}
