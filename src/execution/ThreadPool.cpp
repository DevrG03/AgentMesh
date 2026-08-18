#include <agentmesh/execution/ThreadPool.hpp>

namespace agentmesh::execution {

ThreadPool::ThreadPool(std::size_t numThreads) {
    if (numThreads == 0) {
        numThreads = 1;
    }
    workers_.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex_);
                    this->cv_.wait(lock, [this]() {
                        return this->stop_.load() || !this->tasks_.empty();
                    });

                    if (this->stop_.load() && this->tasks_.empty()) {
                        return;
                    }

                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }

                // Execute task outside the lock
                if (task) {
                    try {
                        task();
                    } catch (...) {
                        // Prevent unhandled task exceptions from killing worker threads
                    }
                }
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (stop_.load()) {
            return;
        }
        tasks_.push(std::move(task));
    }
    cv_.notify_one();
}

void ThreadPool::shutdown() {
    bool expected = false;
    if (stop_.compare_exchange_strong(expected, true)) {
        cv_.notify_all();
        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
}

} // namespace agentmesh::execution
