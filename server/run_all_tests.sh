#!/bin/bash
##############################################################################
# run_all_tests.sh
# 
# Comprehensive test script that:
# 1. Runs unit tests (no server needed)
# 2. Starts server automatically
# 3. Runs integration tests
# 4. Cleans up server process
# 5. Reports results
##############################################################################

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"
TEST_BINARY="$BUILD_DIR/utest/ipc_tests"
SERVER_BINARY="$BUILD_DIR/app/ipc_server"
SERVER_PID=""

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  IPC Demo - Comprehensive Test Suite${NC}"
echo -e "${BLUE}======================================${NC}"
echo ""

# Function to cleanup server on exit
cleanup() {
    if [ ! -z "$SERVER_PID" ]; then
        echo -e "${YELLOW}Stopping server (PID: $SERVER_PID)...${NC}"
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
    fi
}

# Set trap to cleanup on exit
trap cleanup EXIT INT TERM

# Check if test binary exists
if [ ! -f "$TEST_BINARY" ]; then
    echo -e "${RED}Error: Test binary not found at $TEST_BINARY${NC}"
    echo -e "${YELLOW}Please build the project first:${NC}"
    echo -e "  cd $BUILD_DIR && make"
    exit 1
fi

# Check if server binary exists
if [ ! -f "$SERVER_BINARY" ]; then
    echo -e "${RED}Error: Server binary not found at $SERVER_BINARY${NC}"
    exit 1
fi

##############################################################################
# Phase 1: Unit Tests (no server required)
##############################################################################

echo -e "${BLUE}[1/3] Running Unit Tests (no server required)...${NC}"
echo ""

if $TEST_BINARY --gtest_filter="-*Integration*:*ChannelIntegration*:*MultiThreadingIntegration*" \
                --gtest_color=yes \
                --gtest_brief=1; then
    echo -e "${GREEN}✓ Unit tests passed${NC}"
    UNIT_RESULT=0
else
    echo -e "${RED}✗ Unit tests failed${NC}"
    UNIT_RESULT=1
fi

echo ""

##############################################################################
# Phase 2: Start Server
##############################################################################

echo -e "${BLUE}[2/3] Starting IPC Server...${NC}"

# Kill any existing server
pkill -f ipc_server 2>/dev/null || true
sleep 1

# Start server in background
$SERVER_BINARY > /tmp/ipc_server.log 2>&1 &
SERVER_PID=$!

echo -e "${GREEN}✓ Server started (PID: $SERVER_PID)${NC}"

# Wait for server to be ready
echo -n "Waiting for server to be ready"
for i in {1..10}; do
    if [ -S /tmp/ipc_demo.sock ]; then
        echo ""
        echo -e "${GREEN}✓ Server is ready${NC}"
        break
    fi
    echo -n "."
    sleep 0.5
done

if [ ! -S /tmp/ipc_demo.sock ]; then
    echo ""
    echo -e "${RED}✗ Server failed to start${NC}"
    echo -e "${YELLOW}Server log:${NC}"
    cat /tmp/ipc_server.log
    exit 1
fi

echo ""

##############################################################################
# Phase 3: Integration Tests (with server)
##############################################################################

echo -e "${BLUE}[3/3] Running Integration Tests (with server)...${NC}"
echo ""

if $TEST_BINARY --gtest_filter="*Integration*" \
                --gtest_color=yes \
                --gtest_brief=1; then
    echo -e "${GREEN}✓ Integration tests passed${NC}"
    INTEGRATION_RESULT=0
else
    echo -e "${RED}✗ Integration tests failed${NC}"
    INTEGRATION_RESULT=1
fi

echo ""

##############################################################################
# Summary
##############################################################################

echo -e "${BLUE}======================================${NC}"
echo -e "${BLUE}  Test Summary${NC}"
echo -e "${BLUE}======================================${NC}"

if [ $UNIT_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ Unit Tests: PASSED${NC}"
else
    echo -e "${RED}✗ Unit Tests: FAILED${NC}"
fi

if [ $INTEGRATION_RESULT -eq 0 ]; then
    echo -e "${GREEN}✓ Integration Tests: PASSED${NC}"
else
    echo -e "${RED}✗ Integration Tests: FAILED${NC}"
fi

echo ""

if [ $UNIT_RESULT -eq 0 ] && [ $INTEGRATION_RESULT -eq 0 ]; then
    echo -e "${GREEN}======================================${NC}"
    echo -e "${GREEN}  ALL TESTS PASSED! ✓${NC}"
    echo -e "${GREEN}======================================${NC}"
    exit 0
else
    echo -e "${RED}======================================${NC}"
    echo -e "${RED}  SOME TESTS FAILED ✗${NC}"
    echo -e "${RED}======================================${NC}"
    exit 1
fi
