cmake_minimum_required(VERSION 3.10)

##############################################################################
# IPC_SYNC - Unified Client Connector Library
#
# Single shared library containing ALL client-side code:
#   - ByteBuffer (serialization)
#   - Protocol (constants)
#   - Channel (communication layer)
#   - CalculatorClient (calculator proxy)
#   - TimeClient (time service proxy)
#
# Clients link against ONE library: libipc_sync.so
# Clients include headers from: ipc_sync/*.hpp
##############################################################################

# Find threads package (required for std::mutex, std::atomic)
find_package(Threads REQUIRED)

# Create unified shared library
add_library(ipc_sync SHARED
    src/ByteBuffer.cpp
    src/Channel.cpp
    src/CalculatorClient.cpp
    src/TimeClient.cpp
)

# Link dependencies
target_link_libraries(ipc_sync PUBLIC
    Threads::Threads
)

# Include directories
target_include_directories(ipc_sync PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Set library properties
set_target_properties(ipc_sync PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "ipc_sync"
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)


#############################################
# copy ipc_sync.so and header shared folder so that client can use it
#############################################

# Create directories if they don't exist
add_custom_command(TARGET ipc_sync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/libs/
    COMMAND ${CMAKE_COMMAND} -E make_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/include/
    COMMENT "Creating shared directories if needed"
)

# Copy the shared library and symlinks
add_custom_command(TARGET ipc_sync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:ipc_sync>
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/libs/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_SONAME_FILE:ipc_sync>
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/libs/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_LINKER_FILE:ipc_sync>
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/libs/
    COMMENT "Copying ipc_sync.so and symlinks to shared/libs/"
)

# Copy the headers
add_custom_command(TARGET ipc_sync POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../shared/include
    COMMENT "Copying ipc_sync headers to shared/include/"
)
