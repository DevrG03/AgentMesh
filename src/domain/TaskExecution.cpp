#include <agentmesh/domain/TaskExecution.hpp>

namespace agentmesh::domain {

TaskExecution::TaskExecution(TaskId id) : id_(std::move(id)) {}

utils::Result<void> TaskExecution::transitionTo(TaskState newState) {
    if (!isValidTaskTransition(state_, newState)) {
        return utils::Result<void>::error(
            "Invalid task transition from " + std::string(toString(state_)) +
            " to " + std::string(toString(newState)) + " for Task: " + id_.value()
        );
    }
    state_ = newState;
    return utils::Result<void>::success();
}

void TaskExecution::markStarted() {
    state_ = TaskState::Running;
    attempt_++;
    startedAt_ = std::chrono::system_clock::now();
}

void TaskExecution::markCompleted(TaskResult res) {
    state_ = TaskState::Completed;
    completedAt_ = std::chrono::system_clock::now();
    result_ = std::move(res);
}

void TaskExecution::markFailed(TaskResult res) {
    state_ = TaskState::Failed;
    completedAt_ = std::chrono::system_clock::now();
    result_ = std::move(res);
}

} // namespace agentmesh::domain
