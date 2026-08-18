#ifndef AGENTMESH_PERSISTENCE_IN_MEMORY_STATE_REPOSITORY_HPP
#define AGENTMESH_PERSISTENCE_IN_MEMORY_STATE_REPOSITORY_HPP

#include <unordered_map>
#include <mutex>
#include <cstddef>
#include <agentmesh/persistence/IStateRepository.hpp>

namespace agentmesh::persistence {

/// @brief Thread-safe in-memory implementation of IStateRepository (SRP & LSP).
class InMemoryStateRepository : public IStateRepository {
public:
    InMemoryStateRepository() = default;
    ~InMemoryStateRepository() override = default;

    // Non-copyable, non-movable to preserve mutex thread safety
    InMemoryStateRepository(const InMemoryStateRepository&) = delete;
    InMemoryStateRepository& operator=(const InMemoryStateRepository&) = delete;
    InMemoryStateRepository(InMemoryStateRepository&&) = delete;
    InMemoryStateRepository& operator=(InMemoryStateRepository&&) = delete;

    void saveWorkflow(const domain::WorkflowExecution& workflow) override;
    std::optional<domain::WorkflowExecution> loadWorkflow(const domain::WorkflowId& id) const override;
    utils::Result<void> updateTaskState(
        const domain::WorkflowId& workflowId,
        const domain::TaskId& taskId,
        domain::TaskState state
    ) override;
    std::vector<domain::WorkflowExecution> loadActiveWorkflows() const override;

    /// @brief Clears all stored records (useful for test resets).
    void clear();

    /// @brief Returns the total number of stored workflows.
    [[nodiscard]] std::size_t size() const noexcept;

private:
    mutable std::mutex mutex_;
    std::unordered_map<domain::WorkflowId, domain::WorkflowExecution> workflows_;
};

} // namespace agentmesh::persistence

#endif // AGENTMESH_PERSISTENCE_IN_MEMORY_STATE_REPOSITORY_HPP
