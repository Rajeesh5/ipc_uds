cmake_minimum_required(VERSION 3.10)

# Server services (internal use - STATIC)
add_library(ipc_services STATIC
    src/CalculatorService.cpp
    src/TimeService.cpp
)

target_link_libraries(ipc_services PUBLIC
    ipc_server_core
    ipc_sync
)

target_include_directories(ipc_services PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/server_core/include>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/ipc_sync/include>
)
