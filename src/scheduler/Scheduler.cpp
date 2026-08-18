#include <agentmesh/scheduler/Scheduler.hpp>
#include <agentmesh/graph/GraphValidator.hpp>

namespace agentmesh::scheduler {

Scheduler::Scheduler(
    std::shared_ptr<IReadyQueue> readyQueue,
    std::shared_ptr<execution::ITaskDispatcher> dispatcher,
    std::shared_ptr<persistence::IStateRepository> repository
) : readyQueue_(std::move(readyQueue)),
    dispatcher_(std::move(dispatcher)),
    repository_(std::move(repository)) {}

utils::Result<domain::WorkflowId> Scheduler::submit(const domain::WorkflowDefinition& workflowDef) {
    if (!readyQueue_ || !dispatcher_ || !repository_) {
        return utils::Result<domain::WorkflowId>::error("Scheduler missing dependencies");
    }

    graph::WorkflowGraph graph(workflowDef);
    auto validationResult = graph::GraphValidator::validate(graph);
    if (!validationResult.isSuccess()) {
        return utils::Result<domain::WorkflowId>::error(
            "Graph validation failed for workflow " + workflowDef.id().value() + ": " + validationResult.error()
        );
    }

    domain::WorkflowExecution execution(workflowDef);
    auto transitionRes = execution.transitionTo(domain::WorkflowState::Running);
    if (!transitionRes.isSuccess()) {
        return utils::Result<domain::WorkflowId>::error(transitionRes.error());
    }

    repository_->saveWorkflow(execution);

    {
        std::lock_guard<std::mutex> lock(mutex_);
        registeredDefinitions_.insert_or_assign(workflowDef.id(), workflowDef);

        WorkflowContext ctx{
            .definition = workflowDef,
            .graph = graph,
            .remainingDependencies = graph.getInDegrees(),
            .completedTasksCount = 0,
            .failed = false
        };

        for (const auto& [taskId, _] : workflowDef.tasks()) {
            taskToWorkflow_[taskId] = workflowDef.id();
        }

        // Enqueue initial root tasks (in-degree == 0)
        for (const auto& [taskId, inDegree] : ctx.remainingDependencies) {
            if (inDegree == 0) {
                repository_->updateTaskState(workflowDef.id(), taskId, domain::TaskState::Ready);
                auto taskDef = workflowDef.getTask(taskId);
                int priority = taskDef ? taskDef->priority() : 0;
                readyQueue_->push(ReadyTask{taskId, priority});
            }
        }

        activeWorkflows_.insert_or_assign(workflowDef.id(), std::move(ctx));
    }

    drainQueue();

    return utils::Result<domain::WorkflowId>::success(workflowDef.id());
}

void Scheduler::registerDefinition(const domain::WorkflowDefinition& workflowDef) {
    std::lock_guard<std::mutex> lock(mutex_);
    registeredDefinitions_.insert_or_assign(workflowDef.id(), workflowDef);
}

utils::Result<std::size_t> Scheduler::recover(const std::vector<domain::WorkflowDefinition>& knownDefinitions) {
    if (!readyQueue_ || !dispatcher_ || !repository_) {
        return utils::Result<std::size_t>::error("Scheduler missing dependencies");
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& def : knownDefinitions) {
            registeredDefinitions_.insert_or_assign(def.id(), def);
        }
    }

    auto activeWorkflows = repository_->loadActiveWorkflows();
    std::size_t recoveredCount = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& activeWf : activeWorkflows) {
            auto defIt = registeredDefinitions_.find(activeWf.id());
            if (defIt == registeredDefinitions_.end()) {
                continue; // Skip if definition is unknown
            }

            const auto& def = defIt->second;
            graph::WorkflowGraph graph(def);
            auto remainingDeps = graph.getInDegrees();
            std::size_t completedCount = 0;
            bool hasFailure = false;

            // 1. Reconstruct dependency counters based on completed tasks
            for (const auto& [taskId, taskExec] : activeWf.taskExecutions()) {
                taskToWorkflow_[taskId] = activeWf.id();

                if (taskExec.state() == domain::TaskState::Completed) {
                    completedCount++;
                    for (const auto& succId : graph.getSuccessors(taskId)) {
                        auto succIt = remainingDeps.find(succId);
                        if (succIt != remainingDeps.end() && succIt->second > 0) {
                            succIt->second--;
                        }
                    }
                } else if (taskExec.state() == domain::TaskState::Running) {
                    // At-least-once rule: Reset interrupted Running tasks to Ready
                    repository_->updateTaskState(activeWf.id(), taskId, domain::TaskState::Ready);
                } else if (taskExec.state() == domain::TaskState::Failed) {
                    hasFailure = true;
                }
            }

            // 2. Finalize workflow if all tasks finished or failed
            if (completedCount == def.tasks().size()) {
                auto wfOpt = repository_->loadWorkflow(activeWf.id());
                if (wfOpt) {
                    auto wf = wfOpt.value();
                    wf.transitionTo(domain::WorkflowState::Completed);
                    repository_->saveWorkflow(wf);
                }
                continue;
            }

            if (hasFailure) {
                auto wfOpt = repository_->loadWorkflow(activeWf.id());
                if (wfOpt) {
                    auto wf = wfOpt.value();
                    wf.transitionTo(domain::WorkflowState::Failed);
                    repository_->saveWorkflow(wf);
                }
                continue;
            }

            // 3. Find and enqueue runnable tasks (in-degree == 0 and not yet completed)
            for (const auto& [taskId, inDegree] : remainingDeps) {
                auto taskExecOpt = activeWf.getTaskExecution(taskId);
                domain::TaskState currentState = taskExecOpt ? taskExecOpt->state() : domain::TaskState::Pending;

                if (inDegree == 0 && currentState != domain::TaskState::Completed && currentState != domain::TaskState::Failed) {
                    repository_->updateTaskState(activeWf.id(), taskId, domain::TaskState::Ready);
                    auto taskDef = def.getTask(taskId);
                    int priority = taskDef ? taskDef->priority() : 0;
                    readyQueue_->push(ReadyTask{taskId, priority});
                }
            }

            WorkflowContext ctx{
                .definition = def,
                .graph = graph,
                .remainingDependencies = std::move(remainingDeps),
                .completedTasksCount = completedCount,
                .failed = false
            };

            activeWorkflows_.insert_or_assign(activeWf.id(), std::move(ctx));
            recoveredCount++;
        }
    }

    drainQueue();
    return utils::Result<std::size_t>::success(recoveredCount);
}

void Scheduler::onTaskCompleted(const domain::TaskId& taskId, [[maybe_unused]] const domain::TaskResult& result) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto wfIt = taskToWorkflow_.find(taskId);
        if (wfIt == taskToWorkflow_.end()) {
            return;
        }
        const auto wfId = wfIt->second;
        auto ctxIt = activeWorkflows_.find(wfId);
        if (ctxIt == activeWorkflows_.end()) {
            return;
        }

        auto& ctx = ctxIt->second;
        repository_->updateTaskState(wfId, taskId, domain::TaskState::Completed);
        ctx.completedTasksCount++;

        // Decrement in-degree for all downstream dependent tasks in O(1)
        for (const auto& succId : ctx.graph.getSuccessors(taskId)) {
            auto& remaining = ctx.remainingDependencies[succId];
            remaining--;
            if (remaining == 0) {
                repository_->updateTaskState(wfId, succId, domain::TaskState::Ready);
                auto succTaskDef = ctx.definition.getTask(succId);
                int priority = succTaskDef ? succTaskDef->priority() : 0;
                readyQueue_->push(ReadyTask{succId, priority});
            }
        }

        // Check if all tasks in the workflow have completed
        if (ctx.completedTasksCount == ctx.definition.tasks().size()) {
            auto wfOpt = repository_->loadWorkflow(wfId);
            if (wfOpt) {
                auto wf = wfOpt.value();
                wf.transitionTo(domain::WorkflowState::Completed);
                repository_->saveWorkflow(wf);
            }
            activeWorkflows_.erase(ctxIt);
        }
    }

    drainQueue();
}

void Scheduler::onTaskFailed(const domain::TaskId& taskId, [[maybe_unused]] const domain::TaskResult& result) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto wfIt = taskToWorkflow_.find(taskId);
        if (wfIt == taskToWorkflow_.end()) {
            return;
        }
        const auto wfId = wfIt->second;
        auto ctxIt = activeWorkflows_.find(wfId);
        if (ctxIt == activeWorkflows_.end()) {
            return;
        }

        auto& ctx = ctxIt->second;
        ctx.failed = true;
        repository_->updateTaskState(wfId, taskId, domain::TaskState::Failed);

        auto wfOpt = repository_->loadWorkflow(wfId);
        if (wfOpt) {
            auto wf = wfOpt.value();
            wf.transitionTo(domain::WorkflowState::Failed);
            repository_->saveWorkflow(wf);
        }
        activeWorkflows_.erase(ctxIt);
    }
}

void Scheduler::drainQueue() {
    std::vector<domain::TaskDefinition> tasksToDispatch;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!readyQueue_->empty()) {
            auto taskIdOpt = readyQueue_->pop();
            if (!taskIdOpt) {
                break;
            }
            const auto& taskId = taskIdOpt.value();
            auto wfIt = taskToWorkflow_.find(taskId);
            if (wfIt == taskToWorkflow_.end()) {
                continue;
            }
            const auto& wfId = wfIt->second;
            auto ctxIt = activeWorkflows_.find(wfId);
            if (ctxIt == activeWorkflows_.end()) {
                continue;
            }
            auto taskDefOpt = ctxIt->second.definition.getTask(taskId);
            if (!taskDefOpt) {
                continue;
            }

            repository_->updateTaskState(wfId, taskId, domain::TaskState::Running);
            tasksToDispatch.push_back(std::move(taskDefOpt.value()));
        }
    }

    // Dispatch tasks outside the lock to prevent self-deadlock with synchronous executors
    for (const auto& taskDef : tasksToDispatch) {
        dispatcher_->dispatch(taskDef, [this](const domain::TaskId& id, const domain::TaskResult& res) {
            if (res.isSuccess()) {
                this->onTaskCompleted(id, res);
            } else {
                this->onTaskFailed(id, res);
            }
        });
    }
}

std::optional<domain::WorkflowExecution> Scheduler::getWorkflowExecution(const domain::WorkflowId& id) const {
    if (!repository_) {
        return std::nullopt;
    }
    return repository_->loadWorkflow(id);
}

std::size_t Scheduler::activeWorkflowsCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return activeWorkflows_.size();
}

void Scheduler::shutdown() {
    if (dispatcher_) {
        dispatcher_->shutdown();
    }
}

} // namespace agentmesh::scheduler