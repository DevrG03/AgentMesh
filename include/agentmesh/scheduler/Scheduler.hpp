#ifndef AGENTMESH_SCHEDULER_SCHEDULER_HPP
#define AGENTMESH_SCHEDULER_SCHEDULER_HPP

#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <optional>
#include <cstddef>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/TaskState.hpp>
#include <agentmesh/domain/WorkflowState.hpp>
#include <agentmesh/graph/WorkflowGraph.hpp>
#include <agentmesh/scheduler/IReadyQueue.hpp>
#include <agentmesh/execution/ITaskDispatcher.hpp>
#include <agentmesh/persistence/IStateRepository.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::scheduler {

/// @brief Central workflow orchestration engine (SRP, DIP & OCP).
class Scheduler {
public:
    Scheduler(
        std::shared_ptr<IReadyQueue> readyQueue,
        std::shared_ptr<execution::ITaskDispatcher> dispatcher,
        std::shared_ptr<persistence::IStateRepository> repository
    );
    ~Scheduler() = default;

    // Non-copyable, non-movable
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;
    Scheduler(Scheduler&&) = delete;
    Scheduler& operator=(Scheduler&&) = delete;

    /// @brief Submits a workflow for validation, persistence, and execution.
    /// @return Result with WorkflowId on success, or Error on invalid DAG / cycles.
    utils::Result<domain::WorkflowId> submit(const domain::WorkflowDefinition& workflowDef);

    /// @brief Registers a workflow definition without immediately executing it (useful before recovery).
    void registerDefinition(const domain::WorkflowDefinition& workflowDef);

    /// @brief Recovers all incomplete workflows from the state repository after a crash (Milestone 8).
    /// @param knownDefinitions Optional vector of workflow definitions used to reconstruct graphs.
    /// @return The count of successfully recovered and resumed workflows.
    utils::Result<std::size_t> recover(const std::vector<domain::WorkflowDefinition>& knownDefinitions = {});

    /// @brief Callback invoked when a dispatched task completes successfully.
    void onTaskCompleted(const domain::TaskId& taskId, const domain::TaskResult& result = domain::TaskResult::success());

    /// @brief Callback invoked when a dispatched task fails.
    void onTaskFailed(const domain::TaskId& taskId, const domain::TaskResult& result);

    /// @brief Retrieves the current persisted execution snapshot of a workflow.
    [[nodiscard]] std::optional<domain::WorkflowExecution> getWorkflowExecution(const domain::WorkflowId& id) const;

    /// @brief Returns the count of actively running workflows.
    [[nodiscard]] std::size_t activeWorkflowsCount() const;

    /// @brief Gracefully shuts down the underlying task dispatcher.
    void shutdown();

private:
    struct WorkflowContext {
        domain::WorkflowDefinition definition;
        graph::WorkflowGraph graph;
        std::unordered_map<domain::TaskId, int> remainingDependencies;
        std::size_t completedTasksCount{0};
        bool failed{false};
    };

    std::shared_ptr<IReadyQueue> readyQueue_;
    std::shared_ptr<execution::ITaskDispatcher> dispatcher_;
    std::shared_ptr<persistence::IStateRepository> repository_;

    mutable std::mutex mutex_;
    std::unordered_map<domain::WorkflowId, domain::WorkflowDefinition> registeredDefinitions_;
    std::unordered_map<domain::WorkflowId, WorkflowContext> activeWorkflows_;
    std::unordered_map<domain::TaskId, domain::WorkflowId> taskToWorkflow_;

    /// @brief Pops ready tasks from the queue and dispatches them to workers.
    void drainQueue();
};

} // namespace agentmesh::scheduler

#endif // AGENTMESH_SCHEDULER_SCHEDULER_HPP
