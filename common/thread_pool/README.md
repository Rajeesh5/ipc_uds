# ThreadPool Library

## Overview

A simple, thread-safe thread pool implementation based on **"C++ Concurrency in Action" by Anthony Williams, Chapter 9.1 - Simple Thread Pool**.

This library provides a generic thread pool for executing asynchronous tasks using a queue of work items and a fixed number of worker threads.

## Features

- **Fixed-size thread pool**: Create a pool with a specified number of worker threads
- **Type-safe task submission**: Submit any callable (function, lambda, functor) with arguments
- **Future-based results**: Get `std::future` for each submitted task to retrieve results
- **Thread-safe**: All operations are protected by mutexes
- **Exception handling**: Worker threads catch and log exceptions from tasks
- **Clean shutdown**: Destructor ensures all threads are joined properly

## API Reference

### Constructor

```cpp
ThreadPool(size_t num_threads)
```

Creates a thread pool with the specified number of worker threads.

**Parameters:**
- `num_threads`: Number of worker threads to create (must be > 0)

**Throws:**
- `std::invalid_argument` if `num_threads` is 0

### Submit Task

```cpp
template<typename F, typename... Args>
auto Submit(F&& f, Args&&... args) -> std::future<return_type>
```

Submit a task for asynchronous execution.

**Parameters:**
- `f`: Callable object (function, lambda, functor)
- `args...`: Arguments to pass to the callable

**Returns:**
- `std::future<return_type>`: Future to retrieve the result

**Example:**
```cpp
ThreadPool pool(4);

// Submit a task and get result
auto future = pool.Submit([]() {
    return 42;
});

int result = future.get(); // result == 42
```

### Get Pending Task Count

```cpp
size_t GetPendingTaskCount() const
```

Returns the number of tasks waiting in the queue (not including currently executing tasks).

**Returns:**
- Number of pending tasks

### Shutdown

```cpp
void Shutdown()
```

Stops the thread pool and waits for all worker threads to finish. Called automatically by destructor.

## Usage Examples

### Basic Usage

```cpp
#include "thread_pool/ThreadPool.hpp"
#include <iostream>

int main() {
    // Create a thread pool with 4 worker threads
    thread_pool::ThreadPool pool(4);
    
    // Submit tasks
    auto future1 = pool.Submit([]() {
        std::cout << "Task 1 running\n";
        return 100;
    });
    
    auto future2 = pool.Submit([](int x, int y) {
        return x + y;
    }, 10, 20);
    
    // Get results
    int result1 = future1.get(); // 100
    int result2 = future2.get(); // 30
    
    std::cout << "Results: " << result1 << ", " << result2 << "\n";
    
    return 0;
}
```

### With Custom Functions

```cpp
int compute(int a, int b) {
    return a * b;
}

int main() {
    thread_pool::ThreadPool pool(2);
    
    auto future = pool.Submit(compute, 5, 7);
    int result = future.get(); // 35
    
    return 0;
}
```

### Monitoring Queue

```cpp
thread_pool::ThreadPool pool(2);

for (int i = 0; i < 10; ++i) {
    pool.Submit([i]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "Task " << i << "\n";
    });
}

std::cout << "Pending tasks: " << pool.GetPendingTaskCount() << "\n";
```

## Building

### As Standalone Library

```bash
cd common/thread_pool
mkdir build && cd build
cmake ..
make
```

This will generate:
- `libthread_pool.so` (shared library)
- `libthread_pool.so.1` (symlink)
- `libthread_pool.so.1.0.0` (versioned library)

### Integration in CMake Project

Add to your `CMakeLists.txt`:

```cmake
add_subdirectory(common/thread_pool)

target_link_libraries(your_target PRIVATE thread_pool)
```

## Implementation Details

- **Reference**: Chapter 9.1 of "C++ Concurrency in Action" by Anthony Williams
- **Pattern**: Simple thread pool with task queue
- **Thread Safety**: Uses `std::mutex` and `std::condition_variable`
- **Task Queue**: `std::queue<std::function<void()>>`
- **Result Handling**: `std::future` and `std::packaged_task`

## License

Part of the IPC UDS project.

## Notes

This is the **simple thread pool** implementation from Chapter 9.1. It does not include:
- Work stealing (Chapter 9.3)
- Thread interruption (Chapter 9.2)
- Dynamic thread pool sizing
- Task priorities

These features may be added in future increments.
