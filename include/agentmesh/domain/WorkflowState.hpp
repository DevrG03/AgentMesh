#ifndef AGENTMESH_DOMAIN_WORKFLOW_STATE_HPP
#define AGENTMESH_DOMAIN_WORKFLOW_STATE_HPP

#include <string_view>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::domain {

enum class WorkflowState {
    Pending,
    Running,
    Completed,
    Failed
};

[[nodiscard]] constexpr std::string_view toString(WorkflowState state) noexcept {
    switch (state) {
        case WorkflowState::Pending:   return "Pending";
        case WorkflowState::Running:   return "Running";
        case WorkflowState::Completed: return "Completed";
        case WorkflowState::Failed:    return "Failed";
    }
    return "Unknown";
}

[[nodiscard]] inline utils::Result<WorkflowState> workflowStateFromString(std::string_view str) noexcept {
    if (str == "Pending")   return utils::Result<WorkflowState>::success(WorkflowState::Pending);
    if (str == "Running")   return utils::Result<WorkflowState>::success(WorkflowState::Running);
    if (str == "Completed") return utils::Result<WorkflowState>::success(WorkflowState::Completed);
    if (str == "Failed")    return utils::Result<WorkflowState>::success(WorkflowState::Failed);
    return utils::Result<WorkflowState>::error("Invalid WorkflowState string: " + std::string(str));
}

/// @brief Validates if a workflow state transition is legal.
[[nodiscard]] constexpr bool isValidWorkflowTransition(WorkflowState from, WorkflowState to) noexcept {
    switch (from) {
        case WorkflowState::Pending:
            return to == WorkflowState::Running;
        case WorkflowState::Running:
            return to == WorkflowState::Completed || to == WorkflowState::Failed;
        case WorkflowState::Completed:
        case WorkflowState::Failed:
            return false; // Terminal states
    }
    return false;
}

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_WORKFLOW_STATE_HPP
