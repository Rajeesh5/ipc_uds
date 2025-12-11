cmake_minimum_required(VERSION 3.10)
project(CommonLibraries)

# Add thread_pool subdirectory
add_subdirectory(thread_pool)

# Add unit tests (optional, can be disabled with -DBUILD_COMMON_TESTS=OFF)
option(BUILD_COMMON_TESTS "Build common library unit tests" ON)

if(BUILD_COMMON_TESTS)
    enable_testing()
    add_subdirectory(utest)
endif()
