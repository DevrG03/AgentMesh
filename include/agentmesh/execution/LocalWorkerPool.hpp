#ifndef AGENTMESH_EXECUTION_LOCAL_WORKER_POOL_HPP
#define AGENTMESH_EXECUTION_LOCAL_WORKER_POOL_HPP

#include <memory>
#include <agentmesh/execution/ITaskDispatcher.hpp>
#include <agentmesh/execution/ITaskExecutor.hpp>
#include <agentmesh/execution/ThreadPool.hpp>

namespace agentmesh::execution {

/// @brief Dispatches tasks to local worker threads via a ThreadPool (SRP & DIP).
class LocalWorkerPool : public ITaskDispatcher {
public:
    explicit LocalWorkerPool(
        std::shared_ptr<ITaskExecutor> executor,
        std::size_t numThreads = std::thread::hardware_concurrency()
    );
    ~LocalWorkerPool() override;

    LocalWorkerPool(const LocalWorkerPool&) = delete;
    LocalWorkerPool& operator=(const LocalWorkerPool&) = delete;
    LocalWorkerPool(LocalWorkerPool&&) = delete;
    LocalWorkerPool& operator=(LocalWorkerPool&&) = delete;

    void dispatch(const domain::TaskDefinition& task, TaskCompletionCallback onComplete) override;
    void shutdown() override;

private:
    std::shared_ptr<ITaskExecutor> executor_;
    ThreadPool threadPool_;
};

} // namespace agentmesh::execution

#endif // AGENTMESH_EXECUTION_LOCAL_WORKER_POOL_HPP
