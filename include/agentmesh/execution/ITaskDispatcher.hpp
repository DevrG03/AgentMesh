#ifndef AGENTMESH_EXECUTION_ITASK_DISPATCHER_HPP
#define AGENTMESH_EXECUTION_ITASK_DISPATCHER_HPP

#include <functional>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/TaskResult.hpp>

namespace agentmesh::execution {

/// @brief Callback invoked when a dispatched task finishes execution.
using TaskCompletionCallback = std::function<void(const domain::TaskId&, const domain::TaskResult&)>;

/// @brief Pure abstract interface for dispatching tasks to worker backends (DIP & OCP).
class ITaskDispatcher {
public:
    virtual ~ITaskDispatcher() = default;

    /// @brief Dispatches a task for asynchronous execution.
    virtual void dispatch(const domain::TaskDefinition& task, TaskCompletionCallback onComplete) = 0;

    /// @brief Gracefully shuts down the dispatcher and all underlying workers.
    virtual void shutdown() = 0;
};

} // namespace agentmesh::execution

#endif // AGENTMESH_EXECUTION_ITASK_DISPATCHER_HPP
