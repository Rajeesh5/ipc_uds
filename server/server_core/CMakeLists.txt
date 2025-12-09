cmake_minimum_required(VERSION 3.10)

# Server core library (internal use - STATIC)
add_library(ipc_server_core STATIC
    src/ServiceManager.cpp
    src/UDSServer.cpp
)

target_link_libraries(ipc_server_core PUBLIC
    ipc_sync
    Threads::Threads
)

target_include_directories(ipc_server_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ipc_sync/include>
    $<INSTALL_INTERFACE:include>
)
