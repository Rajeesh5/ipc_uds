cmake_minimum_required(VERSION 3.10)

##############################################################################
# Unit Tests for IPC Demo Server
#
# Tests cover:
#   - ByteBuffer serialization/deserialization
#   - Protocol constants and frame structure
#   - ServiceManager registration and execution
#   - CalculatorService operations
#   - TimeService functionality
#   - Channel communication (requires running server)
#   - Multithreading and concurrency
#   - Integration tests (end-to-end)
##############################################################################

# Find Google Test
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Test executable
add_executable(ipc_tests
    test_byte_buffer.cpp
    test_protocol.cpp
    test_service_manager.cpp
    test_calculator_service.cpp
    test_time_service.cpp
    test_channel.cpp
    test_multithreading.cpp
    test_integration.cpp
)

target_link_libraries(ipc_tests PRIVATE
    ipc_services
    ipc_server_core
    ipc_sync
    GTest::GTest
    GTest::Main
    Threads::Threads
)

target_include_directories(ipc_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/services/include
    ${PROJECT_SOURCE_DIR}/server_core/include
    ${PROJECT_SOURCE_DIR}/ipc_sync/include
)

# Enable testing
enable_testing()
add_test(NAME ipc_tests COMMAND ipc_tests)

# Coverage flags (optional, for debug builds)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(ipc_tests PRIVATE --coverage)
    target_link_options(ipc_tests PRIVATE --coverage)
endif()
