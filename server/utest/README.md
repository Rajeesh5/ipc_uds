# Unit Tests for IPC Demo Server

This directory contains comprehensive unit tests for the IPC Demo Server.

## Test Coverage

### 1. ByteBuffer Tests (`test_byte_buffer.cpp`)
- Serialization/deserialization of all data types (byte, int, double, string, map)
- Buffer overflow/underflow detection
- Position management
- Mixed data types
- Edge cases (zero values, negative numbers, large strings)

### 2. Protocol Tests (`test_protocol.cpp`)
- Protocol constants validation
- Frame size calculations
- Socket path configuration

### 3. ServiceManager Tests (`test_service_manager.cpp`)
- Service registration
- Service execution
- Duplicate service handling
- Service lookup
- Concurrent registration

### 4. CalculatorService Tests (`test_calculator_service.cpp`)
- All arithmetic operations (add, subtract, multiply, divide)
- Division by zero error handling
- Invalid operation handling
- Edge cases (negative numbers, large numbers, precision)

### 5. TimeService Tests (`test_time_service.cpp`)
- Time retrieval
- Timestamp format validation
- Error handling
- Multiple requests

### 6. Channel Tests (`test_channel.cpp`)
- Connection/disconnection
- Reconnection
- Multiple channels
- Error handling
- **Note**: Some tests require a running server

### 7. Multithreading Tests (`test_multithreading.cpp`)
- Concurrent service execution
- Shared channel access
- Race condition detection
- Stress testing with many threads
- **Note**: Integration tests require a running server

### 8. Integration Tests (`test_integration.cpp`)
- End-to-end calculator operations
- End-to-end time service
- Multiple services on same channel
- Stress tests (1000+ operations)
- Concurrent client scenarios
- **Note**: All tests require a running server

## Building and Running Tests

### Prerequisites
```bash
# Install Google Test
sudo apt-get install libgtest-dev
cd /usr/src/gtest
sudo cmake .
sudo make
sudo cp lib/*.a /usr/lib
```

### Build Tests
```bash
cd server/build
cmake ..
make
```

### Run All Tests (Automated - Recommended)

The easiest way is to use the automated test script:

```bash
cd server
./run_all_tests.sh
```

This script will:
1. ✅ Run all unit tests (no server needed)
2. ✅ Automatically start the server in background
3. ✅ Run all integration tests
4. ✅ Stop the server and cleanup
5. ✅ Show summary of results

### Run Tests Manually

#### Option A: Run Only Unit Tests (No Server Needed)
```bash
cd server/build
./utest/ipc_tests --gtest_filter="-*Integration*:*ChannelIntegration*:*MultiThreadingIntegration*"
```

#### Option B: Run All Tests (Including Integration)

**Terminal 1: Start Server**
```bash
cd server/build
./app/ipc_server
```

**Terminal 2: Run All Tests**
```bash
cd server/build
./utest/ipc_tests
```

**Note:** Integration tests will automatically skip if server is not running.

### Run Specific Test Suites
```bash
# Run only ByteBuffer tests
./utest/ipc_tests --gtest_filter="ByteBufferTest.*"

# Run only unit tests (no integration tests)
./utest/ipc_tests --gtest_filter="-*Integration*:*ChannelIntegration*:*MultiThreadingIntegration*"

# Run only Calculator tests
./utest/ipc_tests --gtest_filter="CalculatorServiceTest.*"

# Run with verbose output
./utest/ipc_tests --gtest_filter="ByteBufferTest.*" --gtest_print_time=1
```

### Run Integration Tests (Requires Running Server)

**Terminal 1: Start Server**
```bash
cd server/build
./app/ipc_server
```

**Terminal 2: Run Integration Tests**
```bash
cd server/build
./utest/ipc_tests --gtest_filter="*Integration*"
```

## Test Results

### Unit Tests (No Server Required)
- **49 tests** covering core functionality
- Run time: ~100ms
- All can be run without server

### Integration Tests (Server Required)
- **24 tests** covering end-to-end scenarios
- Require running server on `/tmp/ipc_demo.sock`
- Tests will skip automatically if server not running

## Continuous Integration

To run tests in CI/CD:

```bash
#!/bin/bash
# Run unit tests (no server needed)
./utest/ipc_tests --gtest_filter="-*Integration*:*ChannelIntegration*:*MultiThreadingIntegration*" --gtest_output=xml:test_results.xml

# Start server in background
./app/ipc_server &
SERVER_PID=$!
sleep 2

# Run integration tests
./utest/ipc_tests --gtest_filter="*Integration*" --gtest_output=xml:integration_results.xml

# Cleanup
kill $SERVER_PID
```

## Code Coverage

To generate code coverage (Debug build):

```bash
# Build with coverage flags
cd server/build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run tests
./utest/ipc_tests

# Generate coverage report
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

## Test Statistics

- **Total Tests**: 73
- **Unit Tests**: 49
- **Integration Tests**: 24
- **Test Execution Time**: ~150ms (unit only), ~2s (with integration)
- **Code Coverage**: >85% (with all tests)

## Adding New Tests

1. Create new test file in `server/utest/test_*.cpp`
2. Add to `CMakeLists.txt` in the `add_executable(ipc_tests ...)` section
3. Include necessary headers
4. Use GoogleTest macros (TEST, TEST_F, ASSERT_*, EXPECT_*)
5. Run `make` to build
6. Run tests to verify

Example:
```cpp
#include <gtest/gtest.h>

TEST(MyNewTest, BasicTest) {
    EXPECT_EQ(1 + 1, 2);
}
```
