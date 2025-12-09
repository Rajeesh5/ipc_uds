cmake_minimum_required(VERSION 3.10)
project(IPC_Demo_Server VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wpedantic -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find threads
find_package(Threads REQUIRED)

#############################################
# Add modules in dependency order
#############################################

# 1. IPC_SYNC - Unified client connector (no dependencies)
#    Contains: ByteBuffer, Protocol, Channel, Calculator, TimeClient
add_subdirectory(ipc_sync)

# 2. Server core (depends on ipc_sync for Protocol/ByteBuffer)
add_subdirectory(server_core)

# 3. Services (depend on server_core and ipc_sync)
add_subdirectory(services)

# 4. Server application (depends on services and server_core)
add_subdirectory(app)

