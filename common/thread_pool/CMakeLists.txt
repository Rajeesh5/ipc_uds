cmake_minimum_required(VERSION 3.10)
project(ThreadPool VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Threads package (required for std::thread)
find_package(Threads REQUIRED)

# Thread pool library sources
set(THREAD_POOL_SOURCES
    src/ThreadPool.cpp
)

# Create shared library
add_library(thread_pool SHARED ${THREAD_POOL_SOURCES})

# Set library version
set_target_properties(thread_pool PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Public include directories
target_include_directories(thread_pool PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link against threads library
target_link_libraries(thread_pool PUBLIC Threads::Threads)

# Install rules
install(TARGETS thread_pool
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)
