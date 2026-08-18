#ifndef AGENTMESH_EXECUTION_ITASK_EXECUTOR_HPP
#define AGENTMESH_EXECUTION_ITASK_EXECUTOR_HPP

#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/TaskResult.hpp>

namespace agentmesh::execution {

/// @brief Pure abstract interface for executing task payloads (SRP & OCP).
class ITaskExecutor {
public:
    virtual ~ITaskExecutor() = default;

    /// @brief Executes a task definition and returns the execution result.
    virtual domain::TaskResult execute(const domain::TaskDefinition& task) = 0;
};

} // namespace agentmesh::execution

#endif // AGENTMESH_EXECUTION_ITASK_EXECUTOR_HPP
