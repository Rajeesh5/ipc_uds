/**
 * @file ThreadPool.hpp
 * @brief Simple thread pool implementation (from C++ Concurrency in Action, Chapter 9.1)
 * 
 * A basic thread pool that maintains a queue of tasks and a fixed number of worker threads.
 * Tasks are submitted as functions and executed by available worker threads.
 */

#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <atomic>
#include <memory>
#include <stdexcept>

namespace thread_pool {

/**
 * @class ThreadPool
 * @brief A simple thread pool for executing tasks asynchronously
 * 
 * Based on the simple thread pool design from "C++ Concurrency in Action" Chapter 9.1.
 * Maintains a fixed number of worker threads and a task queue.
 */
class ThreadPool {
public:
    /**
     * @brief Construct a thread pool with specified number of threads
     * @param num_threads Number of worker threads (default: hardware concurrency)
     * @throws std::invalid_argument if num_threads is 0
     */
    explicit ThreadPool(size_t num_threads = std::thread::hardware_concurrency());
    
    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool();
    
    // Delete copy and move constructors/assignments
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;
    
    /**
     * @brief Submit a task to the thread pool
     * @tparam F Function type
     * @tparam Args Argument types
     * @param f Function to execute
     * @param args Arguments to pass to the function
     * @return std::future containing the result of the function
     * @throws std::runtime_error if thread pool is stopped
     */
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) 
        -> std::future<typename std::result_of<F(Args...)>::type>;
    
    /**
     * @brief Get the number of worker threads
     * @return Number of threads in the pool
     */
    size_t GetThreadCount() const { return workers_.size(); }
    
    /**
     * @brief Get the number of pending tasks in the queue
     * @return Number of tasks waiting to be executed
     */
    size_t GetPendingTaskCount() const;
    
    /**
     * @brief Shutdown the thread pool and wait for all tasks to complete
     * 
     * This is a graceful shutdown - all queued tasks will be executed.
     * After calling this, no new tasks can be submitted.
     */
    void Shutdown();

private:
    // Worker threads
    std::vector<std::thread> workers_;
    
    // Task queue
    std::queue<std::function<void()>> tasks_;
    
    // Synchronization primitives
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    
    // Stop flag
    std::atomic<bool> stop_{false};
    
    /**
     * @brief Worker thread function
     * 
     * Continuously pulls tasks from the queue and executes them.
     * Exits when stop flag is set and queue is empty.
     */
    void WorkerThread();
};

// Template implementation must be in header
template<typename F, typename... Args>
auto ThreadPool::Submit(F&& f, Args&&... args) 
    -> std::future<typename std::result_of<F(Args...)>::type> {
    
    using return_type = typename std::result_of<F(Args...)>::type;
    
    // Create a packaged task
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...)
    );
    
    std::future<return_type> result = task->get_future();
    
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        if (stop_) {
            throw std::runtime_error("ThreadPool: cannot submit task to stopped pool");
        }
        
        // Enqueue the task
        tasks_.emplace([task]() { (*task)(); });
    }
    
    // Notify one waiting thread
    condition_.notify_one();
    
    return result;
}

} // namespace thread_pool
