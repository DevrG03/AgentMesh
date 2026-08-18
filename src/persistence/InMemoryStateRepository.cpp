#include <agentmesh/persistence/InMemoryStateRepository.hpp>

namespace agentmesh::persistence {

void InMemoryStateRepository::saveWorkflow(const domain::WorkflowExecution& workflow) {
    std::lock_guard<std::mutex> lock(mutex_);
    workflows_.insert_or_assign(workflow.id(), workflow);
}

std::optional<domain::WorkflowExecution> InMemoryStateRepository::loadWorkflow(const domain::WorkflowId& id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = workflows_.find(id);
    if (it != workflows_.end()) {
        return it->second;
    }
    return std::nullopt;
}

utils::Result<void> InMemoryStateRepository::updateTaskState(
    const domain::WorkflowId& workflowId,
    const domain::TaskId& taskId,
    domain::TaskState state
) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = workflows_.find(workflowId);
    if (it == workflows_.end()) {
        return utils::Result<void>::error("Workflow " + workflowId.value() + " not found in repository");
    }
    return it->second.updateTaskState(taskId, state);
}

std::vector<domain::WorkflowExecution> InMemoryStateRepository::loadActiveWorkflows() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<domain::WorkflowExecution> activeWorkflows;
    for (const auto& [_, workflow] : workflows_) {
        if (workflow.state() == domain::WorkflowState::Pending ||
            workflow.state() == domain::WorkflowState::Running) {
            activeWorkflows.push_back(workflow);
        }
    }
    return activeWorkflows;
}

void InMemoryStateRepository::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    workflows_.clear();
}

std::size_t InMemoryStateRepository::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return workflows_.size();
}

} // namespace agentmesh::persistence
