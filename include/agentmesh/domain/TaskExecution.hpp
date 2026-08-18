#ifndef AGENTMESH_DOMAIN_TASK_EXECUTION_HPP
#define AGENTMESH_DOMAIN_TASK_EXECUTION_HPP

#include <chrono>
#include <optional>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/TaskState.hpp>
#include <agentmesh/domain/TaskResult.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::domain {

/// @brief Mutable runtime execution state of an individual task.
class TaskExecution {
public:
    explicit TaskExecution(TaskId id);

    [[nodiscard]] const TaskId& id() const noexcept { return id_; }
    [[nodiscard]] TaskState state() const noexcept { return state_; }
    [[nodiscard]] int attempt() const noexcept { return attempt_; }
    [[nodiscard]] const std::optional<TaskResult>& result() const noexcept { return result_; }
    [[nodiscard]] std::chrono::system_clock::time_point startedAt() const noexcept { return startedAt_; }
    [[nodiscard]] std::chrono::system_clock::time_point completedAt() const noexcept { return completedAt_; }

    utils::Result<void> transitionTo(TaskState newState);
    void markStarted();
    void markCompleted(TaskResult res);
    void markFailed(TaskResult res);

private:
    TaskId id_;
    TaskState state_{TaskState::Pending};
    int attempt_{0};
    std::optional<TaskResult> result_;
    std::chrono::system_clock::time_point startedAt_{};
    std::chrono::system_clock::time_point completedAt_{};
};

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_TASK_EXECUTION_HPP
