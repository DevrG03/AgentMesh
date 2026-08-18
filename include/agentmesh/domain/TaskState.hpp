#ifndef AGENTMESH_DOMAIN_TASK_STATE_HPP
#define AGENTMESH_DOMAIN_TASK_STATE_HPP

#include <string_view>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::domain {

enum class TaskState {
    Pending,
    Ready,
    Running,
    Completed,
    Failed
};

[[nodiscard]] constexpr std::string_view toString(TaskState state) noexcept {
    switch (state) {
        case TaskState::Pending:   return "Pending";
        case TaskState::Ready:     return "Ready";
        case TaskState::Running:   return "Running";
        case TaskState::Completed: return "Completed";
        case TaskState::Failed:    return "Failed";
    }
    return "Unknown";
}

[[nodiscard]] inline utils::Result<TaskState> taskStateFromString(std::string_view str) noexcept {
    if (str == "Pending")   return utils::Result<TaskState>::success(TaskState::Pending);
    if (str == "Ready")     return utils::Result<TaskState>::success(TaskState::Ready);
    if (str == "Running")   return utils::Result<TaskState>::success(TaskState::Running);
    if (str == "Completed") return utils::Result<TaskState>::success(TaskState::Completed);
    if (str == "Failed")    return utils::Result<TaskState>::success(TaskState::Failed);
    return utils::Result<TaskState>::error("Invalid TaskState string: " + std::string(str));
}

/// @brief Validates if a state transition is legal in Phase 1 lifecycle.
[[nodiscard]] constexpr bool isValidTaskTransition(TaskState from, TaskState to) noexcept {
    switch (from) {
        case TaskState::Pending:
            return to == TaskState::Ready;
        case TaskState::Ready:
            return to == TaskState::Running;
        case TaskState::Running:
            // In Phase 1 crash recovery, Running -> Ready is allowed
            return to == TaskState::Completed || to == TaskState::Failed || to == TaskState::Ready;
        case TaskState::Completed:
            return false; // Terminal state
        case TaskState::Failed:
            return to == TaskState::Ready; // Retry support
    }
    return false;
}

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_TASK_STATE_HPP
