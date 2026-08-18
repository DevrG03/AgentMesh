#include <agentmesh/domain/WorkflowExecution.hpp>

namespace agentmesh::domain {

WorkflowExecution::WorkflowExecution(const WorkflowDefinition& definition)
    : id_(definition.id()), state_(WorkflowState::Pending) {
    for (const auto& [taskId, _] : definition.tasks()) {
        taskExecutions_.emplace(taskId, TaskExecution(taskId));
    }
}

WorkflowExecution::WorkflowExecution(WorkflowId id, WorkflowState state)
    : id_(std::move(id)), state_(state) {}

utils::Result<void> WorkflowExecution::transitionTo(WorkflowState newState) {
    if (!isValidWorkflowTransition(state_, newState)) {
        return utils::Result<void>::error(
            "Invalid workflow transition from " + std::string(toString(state_)) +
            " to " + std::string(toString(newState)) + " for Workflow: " + id_.value()
        );
    }
    state_ = newState;
    if (state_ == WorkflowState::Running && startedAt_ == std::chrono::system_clock::time_point{}) {
        startedAt_ = std::chrono::system_clock::now();
    } else if (state_ == WorkflowState::Completed || state_ == WorkflowState::Failed) {
        completedAt_ = std::chrono::system_clock::now();
    }
    return utils::Result<void>::success();
}

utils::Result<void> WorkflowExecution::updateTaskState(const TaskId& taskId, TaskState newState) {
    auto it = taskExecutions_.find(taskId);
    if (it == taskExecutions_.end()) {
        return utils::Result<void>::error("Task " + taskId.value() + " not found in workflow execution " + id_.value());
    }
    return it->second.transitionTo(newState);
}

bool WorkflowExecution::hasTask(const TaskId& id) const noexcept {
    return taskExecutions_.contains(id);
}

std::optional<TaskExecution> WorkflowExecution::getTaskExecution(const TaskId& id) const {
    auto it = taskExecutions_.find(id);
    if (it != taskExecutions_.end()) {
        return it->second;
    }
    return std::nullopt;
}

TaskExecution* WorkflowExecution::getTaskExecutionMut(const TaskId& id) {
    auto it = taskExecutions_.find(id);
    if (it != taskExecutions_.end()) {
        return &(it->second);
    }
    return nullptr;
}

} // namespace agentmesh::domain
