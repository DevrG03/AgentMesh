#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <agentmesh/scheduler/Scheduler.hpp>
#include <agentmesh/scheduler/PriorityReadyQueue.hpp>
#include <agentmesh/execution/LocalWorkerPool.hpp>
#include <agentmesh/execution/ITaskExecutor.hpp>
#include <agentmesh/persistence/InMemoryStateRepository.hpp>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>

using namespace agentmesh::domain;
using namespace agentmesh::scheduler;
using namespace agentmesh::execution;
using namespace agentmesh::persistence;

class MockSimpleExecutor : public ITaskExecutor {
public:
    TaskResult execute(const TaskDefinition& task) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return TaskResult::success("Executed: " + task.name());
    }
};

void testMidExecutionCrashRecovery() {
    auto repository = std::make_shared<InMemoryStateRepository>();

    // Diamond DAG: A -> (B, C) -> D
    WorkflowDefinition def(WorkflowId("diamond_recovery_wf"), "DiamondPipeline");
    def.addTask(TaskDefinition(TaskId("task_A"), "StepA", {}, 10));
    def.addTask(TaskDefinition(TaskId("task_B"), "StepB", {TaskId("task_A")}, 5));
    def.addTask(TaskDefinition(TaskId("task_C"), "StepC", {TaskId("task_A")}, 5));
    def.addTask(TaskDefinition(TaskId("task_D"), "StepD", {TaskId("task_B"), TaskId("task_C")}, 1));

    // Simulate pre-crash state in database:
    // - Workflow: Running
    // - Task A: Completed
    // - Task C: Completed
    // - Task B: Running (interrupted mid-execution by crash!)
    // - Task D: Pending
    WorkflowExecution execution(def);
    assert(execution.transitionTo(WorkflowState::Running).isSuccess());
    assert(execution.updateTaskState(TaskId("task_A"), TaskState::Ready).isSuccess());
    assert(execution.updateTaskState(TaskId("task_A"), TaskState::Running).isSuccess());
    assert(execution.updateTaskState(TaskId("task_A"), TaskState::Completed).isSuccess());

    assert(execution.updateTaskState(TaskId("task_C"), TaskState::Ready).isSuccess());
    assert(execution.updateTaskState(TaskId("task_C"), TaskState::Running).isSuccess());
    assert(execution.updateTaskState(TaskId("task_C"), TaskState::Completed).isSuccess());

    assert(execution.updateTaskState(TaskId("task_B"), TaskState::Ready).isSuccess());
    assert(execution.updateTaskState(TaskId("task_B"), TaskState::Running).isSuccess());

    repository->saveWorkflow(execution);

    // ==========================================
    // SIMULATE PROCESS RESTART: Fresh Scheduler
    // ==========================================
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockSimpleExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 4);

    Scheduler newScheduler(queue, dispatcher, repository);

    // Trigger crash recovery
    auto recoverRes = newScheduler.recover({def});
    assert(recoverRes.isSuccess());
    assert(recoverRes.value() == 1);

    // Wait for the recovered workflow to finish executing B and D
    for (int i = 0; i < 100; ++i) {
        auto wfOpt = newScheduler.getWorkflowExecution(WorkflowId("diamond_recovery_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto finalWf = newScheduler.getWorkflowExecution(WorkflowId("diamond_recovery_wf"));
    assert(finalWf.has_value());
    assert(finalWf->state() == WorkflowState::Completed);

    assert(finalWf->getTaskExecution(TaskId("task_A"))->state() == TaskState::Completed);
    assert(finalWf->getTaskExecution(TaskId("task_B"))->state() == TaskState::Completed);
    assert(finalWf->getTaskExecution(TaskId("task_C"))->state() == TaskState::Completed);
    assert(finalWf->getTaskExecution(TaskId("task_D"))->state() == TaskState::Completed);

    newScheduler.shutdown();
    std::cout << "[PASSED] testMidExecutionCrashRecovery (Interrupted Task B resumed, D executed to completion)\n";
}

void testRecoveryWithNoActiveWorkflows() {
    auto repository = std::make_shared<InMemoryStateRepository>();
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockSimpleExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 2);

    Scheduler scheduler(queue, dispatcher, repository);

    auto res = scheduler.recover();
    assert(res.isSuccess());
    assert(res.value() == 0);

    scheduler.shutdown();
    std::cout << "[PASSED] testRecoveryWithNoActiveWorkflows (Clean empty state return)\n";
}

int main() {
    std::cout << "=== Running Milestone 8 Process Crash Recovery Integration Tests ===\n";
    testMidExecutionCrashRecovery();
    testRecoveryWithNoActiveWorkflows();
    std::cout << "=== All Milestone 8 Unit & Integration Tests Passed Successfully! ===\n";
    return 0;
}
