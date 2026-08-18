#ifndef AGENTMESH_EXECUTION_THREAD_POOL_HPP
#define AGENTMESH_EXECUTION_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>
#include <cstddef>

namespace agentmesh::execution {

/// @brief General-purpose thread pool with graceful shutdown and exception safety.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t numThreads = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /// @brief Enqueues a generic callable task into the worker pool.
    void submit(std::function<void()> task);

    /// @brief Signals worker threads to stop and joins them cleanly.
    void shutdown();

    [[nodiscard]] std::size_t size() const noexcept { return workers_.size(); }
    [[nodiscard]] bool isRunning() const noexcept { return !stop_.load(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queueMutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
};

} // namespace agentmesh::execution

#endif // AGENTMESH_EXECUTION_THREAD_POOL_HPP
