#ifndef AGENTMESH_PERSISTENCE_ISTATE_REPOSITORY_HPP
#define AGENTMESH_PERSISTENCE_ISTATE_REPOSITORY_HPP

#include <optional>
#include <vector>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/TaskState.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::persistence {

/// @brief Pure abstract interface for state persistence (DIP, ISP & OCP).
class IStateRepository {
public:
    virtual ~IStateRepository() = default;

    /// @brief Saves or overwrites a workflow execution state snapshot.
    virtual void saveWorkflow(const domain::WorkflowExecution& workflow) = 0;

    /// @brief Loads a workflow execution snapshot by its unique ID.
    /// @return The workflow execution if found, std::nullopt otherwise.
    virtual std::optional<domain::WorkflowExecution> loadWorkflow(const domain::WorkflowId& id) const = 0;

    /// @brief Directly updates the state of a specific task within a workflow.
    /// @return Result success, or Result error if the workflow/task does not exist or transition is invalid.
    virtual utils::Result<void> updateTaskState(
        const domain::WorkflowId& workflowId,
        const domain::TaskId& taskId,
        domain::TaskState state
    ) = 0;

    /// @brief Retrieves all active workflows (Pending or Running) for crash recovery and scheduling.
    virtual std::vector<domain::WorkflowExecution> loadActiveWorkflows() const = 0;
};

} // namespace agentmesh::persistence

#endif // AGENTMESH_PERSISTENCE_ISTATE_REPOSITORY_HPP
