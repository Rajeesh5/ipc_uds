cmake_minimum_required(VERSION 3.10)
project(CommonUnitTests)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing at the top level
enable_testing()

# Find Google Test
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Include thread_pool headers
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../thread_pool/include)

# Test sources
set(TEST_SOURCES
    test_thread_pool.cpp
)

# Create test executable
add_executable(common_tests ${TEST_SOURCES})

# Link against thread_pool library and GTest
target_link_libraries(common_tests
    thread_pool
    ${GTEST_LIBRARIES}
    pthread
)

# Add test to CTest
add_test(NAME ThreadPoolTests COMMAND common_tests)
