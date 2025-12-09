cmake_minimum_required(VERSION 3.10)
project(IPC_Client VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find threads (required by shared libraries)
find_package(Threads REQUIRED)

set(IPC_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../shared/include")
set(IPC_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../shared/libs")

#############################################
# Demo client application
# NO local library - uses only shared libs from server
# True client-server separation!
#############################################
add_executable(client
    client_app/main.cpp
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IPC_INCLUDE_DIRS}
)

# Link against installed shared libraries from server
target_link_libraries(client PRIVATE
    ${IPC_LIB_DIR}/libipc_sync.so
    Threads::Threads
)

