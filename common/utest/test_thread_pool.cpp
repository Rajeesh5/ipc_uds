/**
 * @file test_thread_pool.cpp
 * @brief Comprehensive unit tests for ThreadPool library
 * 
 * Test Coverage:
 * - Basic task submission and execution
 * - Future-based result retrieval
 * - Multiple concurrent tasks
 * - Different return types (void, int, string, etc.)
 * - Exception handling in tasks
 * - Thread pool shutdown
 * - Queue monitoring
 * - Stress testing with many tasks
 * - Thread safety verification
 */

#include "thread_pool/ThreadPool.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <stdexcept>

using namespace thread_pool;

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(ThreadPoolTest, ConstructorCreatesThreads) {
    // Test that constructor doesn't throw and creates a valid pool
    EXPECT_NO_THROW({
        ThreadPool pool(4);
    });
}

TEST(ThreadPoolTest, ConstructorRejectsZeroThreads) {
    // Test that zero threads throws invalid_argument
    EXPECT_THROW({
        ThreadPool pool(0);
    }, std::invalid_argument);
}

TEST(ThreadPoolTest, SubmitSimpleLambda) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([]() {
        return 42;
    });
    
    EXPECT_EQ(42, future.get());
}

TEST(ThreadPoolTest, SubmitLambdaWithArguments) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([](int a, int b) {
        return a + b;
    }, 10, 32);
    
    EXPECT_EQ(42, future.get());
}

TEST(ThreadPoolTest, SubmitFunction) {
    ThreadPool pool(2);
    
    auto multiply = [](int x, int y) { return x * y; };
    auto future = pool.Submit(multiply, 6, 7);
    
    EXPECT_EQ(42, future.get());
}

TEST(ThreadPoolTest, SubmitVoidTask) {
    ThreadPool pool(2);
    
    std::atomic<int> counter{0};
    
    auto future = pool.Submit([&counter]() {
        counter++;
    });
    
    future.wait(); // Wait for completion
    EXPECT_EQ(1, counter.load());
}

// ============================================================================
// Return Type Tests
// ============================================================================

TEST(ThreadPoolTest, SubmitReturnsString) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([]() {
        return std::string("Hello, ThreadPool!");
    });
    
    EXPECT_EQ("Hello, ThreadPool!", future.get());
}

TEST(ThreadPoolTest, SubmitReturnsDouble) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([](double x, double y) {
        return x * y;
    }, 3.14, 2.0);
    
    EXPECT_NEAR(6.28, future.get(), 0.01);
}

TEST(ThreadPoolTest, SubmitReturnsVector) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([]() {
        return std::vector<int>{1, 2, 3, 4, 5};
    });
    
    auto result = future.get();
    EXPECT_EQ(5u, result.size());
    EXPECT_EQ(1, result[0]);
    EXPECT_EQ(5, result[4]);
}

// ============================================================================
// Concurrent Execution Tests
// ============================================================================

TEST(ThreadPoolTest, MultipleConcurrentTasks) {
    ThreadPool pool(4);
    
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.Submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return i * 2;
        }));
    }
    
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(i * 2, futures[i].get());
    }
}

TEST(ThreadPoolTest, TasksExecuteInParallel) {
    ThreadPool pool(4);
    
    std::atomic<int> concurrent_count{0};
    std::atomic<int> max_concurrent{0};
    
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.Submit([&concurrent_count, &max_concurrent]() {
            int current = ++concurrent_count;
            
            // Update max if needed
            int expected = max_concurrent.load();
            while (current > expected && 
                   !max_concurrent.compare_exchange_weak(expected, current)) {
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            --concurrent_count;
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    // With 4 threads and 8 tasks, we should see at least 2 concurrent
    EXPECT_GE(max_concurrent.load(), 2);
}

TEST(ThreadPoolTest, ThreadSafety) {
    ThreadPool pool(4);
    
    std::atomic<int> counter{0};
    std::vector<std::future<void>> futures;
    
    // Submit 100 tasks that increment a counter
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.Submit([&counter]() {
            counter++;
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_EQ(100, counter.load());
}

// ============================================================================
// Exception Handling Tests
// ============================================================================

TEST(ThreadPoolTest, TaskThrowsException) {
    ThreadPool pool(2);
    
    auto future = pool.Submit([]() -> int {
        throw std::runtime_error("Test exception");
    });
    
    EXPECT_THROW({
        future.get();
    }, std::runtime_error);
}

TEST(ThreadPoolTest, MultipleTasksWithExceptions) {
    ThreadPool pool(2);
    
    auto future1 = pool.Submit([]() { return 42; });
    auto future2 = pool.Submit([]() -> int {
        throw std::runtime_error("Error");
    });
    auto future3 = pool.Submit([]() { return 100; });
    
    EXPECT_EQ(42, future1.get());
    EXPECT_THROW(future2.get(), std::runtime_error);
    EXPECT_EQ(100, future3.get());
}

TEST(ThreadPoolTest, ExceptionDoesNotCrashPool) {
    ThreadPool pool(2);
    
    // Submit a task that throws
    auto future1 = pool.Submit([]() -> int {
        throw std::logic_error("Test error");
    });
    
    EXPECT_THROW(future1.get(), std::logic_error);
    
    // Pool should still work after exception
    auto future2 = pool.Submit([]() { return 42; });
    EXPECT_EQ(42, future2.get());
}

// ============================================================================
// Shutdown Tests
// ============================================================================

TEST(ThreadPoolTest, ShutdownCompletesRunningTasks) {
    ThreadPool pool(2);
    
    std::atomic<int> completed{0};
    
    auto future1 = pool.Submit([&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        completed++;
    });
    
    auto future2 = pool.Submit([&completed]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        completed++;
    });
    
    pool.Shutdown();
    
    EXPECT_EQ(2, completed.load());
}

TEST(ThreadPoolTest, ShutdownIdempotent) {
    ThreadPool pool(2);
    
    pool.Shutdown();
    EXPECT_NO_THROW({
        pool.Shutdown(); // Should not throw on second call
    });
}

TEST(ThreadPoolTest, DestructorCallsShutdown) {
    std::atomic<int> completed{0};
    
    {
        ThreadPool pool(2);
        
        pool.Submit([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            completed++;
        });
    } // Destructor should wait for task
    
    EXPECT_EQ(1, completed.load());
}

// ============================================================================
// Queue Monitoring Tests
// ============================================================================

TEST(ThreadPoolTest, GetPendingTaskCountInitiallyZero) {
    ThreadPool pool(2);
    EXPECT_EQ(0u, pool.GetPendingTaskCount());
}

TEST(ThreadPoolTest, GetPendingTaskCountTracksQueue) {
    ThreadPool pool(1); // Single thread to control execution
    
    std::atomic<bool> gate{false};
    
    // Submit a blocking task
    auto future1 = pool.Submit([&gate]() {
        while (!gate.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    
    // Give it time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Submit more tasks - these will queue
    auto future2 = pool.Submit([]() { return 1; });
    auto future3 = pool.Submit([]() { return 2; });
    auto future4 = pool.Submit([]() { return 3; });
    
    // Check pending count (should be 3 - not counting the running task)
    size_t pending = pool.GetPendingTaskCount();
    EXPECT_GE(pending, 2u); // At least 2 should be queued
    
    // Release the gate
    gate.store(true);
    
    // Wait for all to complete
    future1.wait();
    future2.wait();
    future3.wait();
    future4.wait();
    
    // Queue should be empty now
    EXPECT_EQ(0u, pool.GetPendingTaskCount());
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(ThreadPoolTest, ManySmallTasks) {
    ThreadPool pool(8);
    
    const int NUM_TASKS = 1000;
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < NUM_TASKS; ++i) {
        futures.push_back(pool.Submit([i]() {
            return i;
        }));
    }
    
    for (int i = 0; i < NUM_TASKS; ++i) {
        EXPECT_EQ(i, futures[i].get());
    }
}

TEST(ThreadPoolTest, LongRunningTasks) {
    ThreadPool pool(4);
    
    std::atomic<int> completed{0};
    std::vector<std::future<void>> futures;
    
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.Submit([&completed]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            completed++;
        }));
    }
    
    for (auto& future : futures) {
        future.wait();
    }
    
    EXPECT_EQ(8, completed.load());
}

TEST(ThreadPoolTest, MixedTaskDurations) {
    ThreadPool pool(4);
    
    std::vector<std::future<int>> futures;
    
    // Mix of fast and slow tasks
    for (int i = 0; i < 20; ++i) {
        int delay = (i % 3 == 0) ? 50 : 5;
        
        futures.push_back(pool.Submit([i, delay]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            return i;
        }));
    }
    
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(i, futures[i].get());
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(ThreadPoolTest, SingleThreadPool) {
    ThreadPool pool(1);
    
    auto future1 = pool.Submit([]() { return 1; });
    auto future2 = pool.Submit([]() { return 2; });
    auto future3 = pool.Submit([]() { return 3; });
    
    EXPECT_EQ(1, future1.get());
    EXPECT_EQ(2, future2.get());
    EXPECT_EQ(3, future3.get());
}

TEST(ThreadPoolTest, LargeThreadPool) {
    ThreadPool pool(32);
    
    std::vector<std::future<int>> futures;
    
    for (int i = 0; i < 64; ++i) {
        futures.push_back(pool.Submit([i]() { return i; }));
    }
    
    for (int i = 0; i < 64; ++i) {
        EXPECT_EQ(i, futures[i].get());
    }
}

TEST(ThreadPoolTest, ResubmitAfterGet) {
    ThreadPool pool(2);
    
    auto future1 = pool.Submit([]() { return 42; });
    EXPECT_EQ(42, future1.get());
    
    // Submit again after getting result
    auto future2 = pool.Submit([]() { return 100; });
    EXPECT_EQ(100, future2.get());
}

TEST(ThreadPoolTest, TaskReturnsReference) {
    ThreadPool pool(2);
    
    std::string str = "Hello";
    
    auto future = pool.Submit([&str]() -> const std::string& {
        return str;
    });
    
    EXPECT_EQ("Hello", future.get());
}

// ============================================================================
// Performance Characteristics Tests
// ============================================================================

TEST(ThreadPoolTest, TasksExecuteFasterWithMoreThreads) {
    const int NUM_TASKS = 16;
    const int TASK_DURATION_MS = 50;
    
    // Single thread pool
    auto start1 = std::chrono::high_resolution_clock::now();
    {
        ThreadPool pool(1);
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < NUM_TASKS; ++i) {
            futures.push_back(pool.Submit([=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(TASK_DURATION_MS));
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start1
    ).count();
    
    // Multi-thread pool
    auto start2 = std::chrono::high_resolution_clock::now();
    {
        ThreadPool pool(4);
        std::vector<std::future<void>> futures;
        
        for (int i = 0; i < NUM_TASKS; ++i) {
            futures.push_back(pool.Submit([=]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(TASK_DURATION_MS));
            }));
        }
        
        for (auto& future : futures) {
            future.wait();
        }
    }
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::high_resolution_clock::now() - start2
    ).count();
    
    // Multi-threaded should be significantly faster
    // With 4 threads, should be roughly 3-4x faster
    EXPECT_LT(duration2, duration1 * 0.7);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
