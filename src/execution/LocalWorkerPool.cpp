#include <agentmesh/execution/LocalWorkerPool.hpp>
#include <chrono>
#include <utility>
#include <stdexcept>

namespace agentmesh::execution {

LocalWorkerPool::LocalWorkerPool(
    std::shared_ptr<ITaskExecutor> executor,
    std::size_t numThreads
) : executor_(std::move(executor)), threadPool_(numThreads) {}

LocalWorkerPool::~LocalWorkerPool() {
    shutdown();
}

void LocalWorkerPool::dispatch(const domain::TaskDefinition& task, TaskCompletionCallback onComplete) {
    threadPool_.submit([this, task, callback = std::move(onComplete)]() {
        const auto startTime = std::chrono::steady_clock::now();
        domain::TaskResult result;

        try {
            if (this->executor_) {
                result = this->executor_->execute(task);
            } else {
                result = domain::TaskResult::failure("No executor registered");
            }
        } catch (const std::exception& ex) {
            const auto endTime = std::chrono::steady_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            result = domain::TaskResult::failure(std::string("Task execution exception: ") + ex.what(), duration);
        } catch (...) {
            const auto endTime = std::chrono::steady_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
            result = domain::TaskResult::failure("Unknown exception occurred during task execution", duration);
        }

        if (callback) {
            callback(task.id(), result);
        }
    });
}

void LocalWorkerPool::shutdown() {
    threadPool_.shutdown();
}

} // namespace agentmesh::execution
