cmake_minimum_required(VERSION 3.10)

# Server executable
add_executable(ipc_server
    main.cpp
)

target_link_libraries(ipc_server PRIVATE
    ipc_services
    ipc_server_core
    ipc_sync
    Threads::Threads
)

target_include_directories(ipc_server PRIVATE
    ${PROJECT_SOURCE_DIR}/services/include
    ${PROJECT_SOURCE_DIR}/server_core/include
    ${PROJECT_SOURCE_DIR}/ipc_sync/include
)

# Install
install(TARGETS ipc_server
    RUNTIME DESTINATION ${IPC_INSTALL_BIN_DIR}
)
