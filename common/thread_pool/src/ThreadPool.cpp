/**
 * @file ThreadPool.cpp
 * @brief Implementation of simple thread pool (from C++ Concurrency in Action, Chapter 9.1)
 */

#include "thread_pool/ThreadPool.hpp"
#include <iostream>

namespace thread_pool {

ThreadPool::ThreadPool(size_t num_threads) {
    if (num_threads == 0) {
        throw std::invalid_argument("ThreadPool: num_threads must be at least 1");
    }
    
    workers_.reserve(num_threads);
    
    // Create worker threads
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this]() { WorkerThread(); });
    }
}

ThreadPool::~ThreadPool() {
    Shutdown();
}

void ThreadPool::WorkerThread() {
    while (true) {
        std::function<void()> task;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            
            // Wait for a task or stop signal
            condition_.wait(lock, [this]() {
                return stop_ || !tasks_.empty();
            });
            
            // Exit if stopping and no more tasks
            if (stop_ && tasks_.empty()) {
                return;
            }
            
            // Get the next task
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }
        
        // Execute the task (outside the lock)
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "[ThreadPool] Task threw exception: " 
                          << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ThreadPool] Task threw unknown exception" 
                          << std::endl;
            }
        }
    }
}

size_t ThreadPool::GetPendingTaskCount() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void ThreadPool::Shutdown() {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (stop_) {
            return; // Already stopped
        }
        stop_ = true;
    }
    
    // Wake up all threads
    condition_.notify_all();
    
    // Wait for all threads to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

} // namespace thread_pool
