#ifndef AGENTMESH_DOMAIN_WORKFLOW_EXECUTION_HPP
#define AGENTMESH_DOMAIN_WORKFLOW_EXECUTION_HPP

#include <chrono>
#include <unordered_map>
#include <optional>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/domain/WorkflowState.hpp>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskExecution.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::domain {

/// @brief Mutable runtime execution state of an entire workflow instance.
class WorkflowExecution {
public:
    explicit WorkflowExecution(const WorkflowDefinition& definition);
    WorkflowExecution(WorkflowId id, WorkflowState state);

    [[nodiscard]] const WorkflowId& id() const noexcept { return id_; }
    [[nodiscard]] WorkflowState state() const noexcept { return state_; }
    [[nodiscard]] std::chrono::system_clock::time_point startedAt() const noexcept { return startedAt_; }
    [[nodiscard]] std::chrono::system_clock::time_point completedAt() const noexcept { return completedAt_; }

    utils::Result<void> transitionTo(WorkflowState newState);
    utils::Result<void> updateTaskState(const TaskId& taskId, TaskState newState);

    [[nodiscard]] bool hasTask(const TaskId& id) const noexcept;
    [[nodiscard]] std::optional<TaskExecution> getTaskExecution(const TaskId& id) const;
    [[nodiscard]] TaskExecution* getTaskExecutionMut(const TaskId& id);
    [[nodiscard]] const std::unordered_map<TaskId, TaskExecution>& taskExecutions() const noexcept {
        return taskExecutions_;
    }

private:
    WorkflowId id_;
    WorkflowState state_{WorkflowState::Pending};
    std::unordered_map<TaskId, TaskExecution> taskExecutions_;
    std::chrono::system_clock::time_point startedAt_{};
    std::chrono::system_clock::time_point completedAt_{};
};

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_WORKFLOW_EXECUTION_HPP
