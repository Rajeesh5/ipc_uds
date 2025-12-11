# Common Library Unit Tests

## Overview

This directory contains unit tests for all common library modules, including:
- **thread_pool**: Simple thread pool implementation (Chapter 9.1)
- *Future modules will be added here*

## Test Suite: ThreadPool

### Test Coverage (45 tests)

#### 1. Basic Functionality (7 tests)
- `ConstructorCreatesThreads`: Verify pool creation
- `ConstructorRejectsZeroThreads`: Validate error handling
- `SubmitSimpleLambda`: Basic lambda execution
- `SubmitLambdaWithArguments`: Lambda with parameters
- `SubmitFunction`: Regular function submission
- `SubmitVoidTask`: Void return type handling

#### 2. Return Type Tests (3 tests)
- `SubmitReturnsString`: String return values
- `SubmitReturnsDouble`: Floating-point return values
- `SubmitReturnsVector`: Complex type return values

#### 3. Concurrent Execution (3 tests)
- `MultipleConcurrentTasks`: Multiple tasks with futures
- `TasksExecuteInParallel`: Verify parallel execution
- `ThreadSafety`: Atomic operations validation

#### 4. Exception Handling (3 tests)
- `TaskThrowsException`: Exception propagation through futures
- `MultipleTasksWithExceptions`: Mixed success/failure tasks
- `ExceptionDoesNotCrashPool`: Pool resilience

#### 5. Shutdown Tests (3 tests)
- `ShutdownCompletesRunningTasks`: Graceful shutdown
- `ShutdownIdempotent`: Multiple shutdown calls
- `DestructorCallsShutdown`: RAII cleanup

#### 6. Queue Monitoring (2 tests)
- `GetPendingTaskCountInitiallyZero`: Initial state
- `GetPendingTaskCountTracksQueue`: Queue size tracking

#### 7. Stress Tests (3 tests)
- `ManySmallTasks`: 1000 quick tasks
- `LongRunningTasks`: Extended duration tasks
- `MixedTaskDurations`: Varied workload

#### 8. Edge Cases (4 tests)
- `SingleThreadPool`: Minimum configuration
- `LargeThreadPool`: 32 threads with 64 tasks
- `ResubmitAfterGet`: Sequential submissions
- `TaskReturnsReference`: Reference return handling

#### 9. Performance (1 test)
- `TasksExecuteFasterWithMoreThreads`: Scalability verification

## Building and Running Tests

### Build Tests

```bash
# From project root
cd common
mkdir -p build && cd build
cmake ..
make
```

This will build:
- `libthread_pool.so` - ThreadPool shared library
- `common_tests` - Test executable

### Run All Tests

```bash
cd common/build/utest
./common_tests
```

### Run Specific Test Suite

```bash
./common_tests --gtest_filter=ThreadPoolTest.*
```

### Run Specific Test

```bash
./common_tests --gtest_filter=ThreadPoolTest.SubmitSimpleLambda
```

### Verbose Output

```bash
./common_tests --gtest_verbose
```

### Run with CTest

```bash
cd common/build
ctest --verbose
```

## Disable Tests

To build without tests:

```bash
cmake .. -DBUILD_COMMON_TESTS=OFF
make
```

## Test Results Format

Tests use GoogleTest framework with color-coded output:
- 🟢 **[  PASSED  ]** - Test succeeded
- 🔴 **[  FAILED  ]** - Test failed
- 🟡 **[ DISABLED ]** - Test skipped

## Expected Output

```
[==========] Running 45 tests from 1 test suite.
[----------] Global test environment set-up.
[----------] 45 tests from ThreadPoolTest
[ RUN      ] ThreadPoolTest.ConstructorCreatesThreads
[       OK ] ThreadPoolTest.ConstructorCreatesThreads (0 ms)
...
[----------] 45 tests from ThreadPoolTest (XXX ms total)

[----------] Global test environment tear-down
[==========] 45 tests from 1 test suite ran. (XXX ms total)
[  PASSED  ] 45 tests.
```

## Adding New Tests

When adding new common modules:

1. Create test file: `test_<module_name>.cpp`
2. Update `common/utest/CMakeLists.txt`:
   ```cmake
   set(TEST_SOURCES
       test_thread_pool.cpp
       test_<new_module>.cpp  # Add here
   )
   ```
3. Link against new module library in `target_link_libraries`

## Continuous Integration

These tests should be run:
- Before committing changes to common libraries
- In CI/CD pipeline for pull requests
- After any thread pool modifications

## Test Development Guidelines

1. **Use descriptive names**: `TestCategory.SpecificBehavior`
2. **One assertion per test**: Focus on single behavior
3. **Clean up resources**: Use RAII, avoid manual cleanup
4. **Test edge cases**: Zero, one, many, maximum
5. **Verify thread safety**: Use atomics for shared state
6. **Document complex tests**: Add comments for non-obvious logic

## Troubleshooting

### Tests Hang
- Check for deadlocks in shutdown
- Verify futures are being retrieved
- Ensure tasks aren't blocking indefinitely

### Intermittent Failures
- Race conditions in concurrent tests
- Timing-dependent assertions
- Insufficient sleep/wait times

### Link Errors
- Verify GoogleTest is installed
- Check thread_pool library is built first
- Ensure proper library paths

## Dependencies

- **GoogleTest**: v1.14.0 or later
- **C++17**: Required for ThreadPool
- **pthread**: POSIX threads library
- **thread_pool**: Common library module

## License

Part of the IPC UDS project.
