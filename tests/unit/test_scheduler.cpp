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

/// @brief Mock executor that tracks execution logs and simulates small latency
class MockTrackingExecutor : public ITaskExecutor {
public:
    TaskResult execute(const TaskDefinition& task) override {
        if (task.name() == "CrashTask") {
            return TaskResult::failure("Deliberate task failure");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return TaskResult::success("Done: " + task.name());
    }
};

void testDiamondWorkflowExecution() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockTrackingExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 4);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    // Diamond DAG: A -> (B, C) -> D
    WorkflowDefinition def(WorkflowId("diamond_wf"), "DiamondPipeline");
    def.addTask(TaskDefinition(TaskId("task_A"), "StepA", {}, 10));
    def.addTask(TaskDefinition(TaskId("task_B"), "StepB", {TaskId("task_A")}, 5));
    def.addTask(TaskDefinition(TaskId("task_C"), "StepC", {TaskId("task_A")}, 5));
    def.addTask(TaskDefinition(TaskId("task_D"), "StepD", {TaskId("task_B"), TaskId("task_C")}, 1));

    auto submitRes = scheduler.submit(def);
    assert(submitRes.isSuccess());

    // Wait for the workflow to finish processing
    for (int i = 0; i < 100; ++i) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("diamond_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto execution = scheduler.getWorkflowExecution(WorkflowId("diamond_wf"));
    assert(execution.has_value());
    assert(execution->state() == WorkflowState::Completed);

    assert(execution->getTaskExecution(TaskId("task_A"))->state() == TaskState::Completed);
    assert(execution->getTaskExecution(TaskId("task_B"))->state() == TaskState::Completed);
    assert(execution->getTaskExecution(TaskId("task_C"))->state() == TaskState::Completed);
    assert(execution->getTaskExecution(TaskId("task_D"))->state() == TaskState::Completed);

    scheduler.shutdown();
    std::cout << "[PASSED] testDiamondWorkflowExecution (Topological A -> (B,C) -> D)\n";
}

void testCycleDetectionRejection() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockTrackingExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 2);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    // Cycle DAG: A -> B -> C -> A
    WorkflowDefinition cyclicDef(WorkflowId("cycle_wf"), "CyclePipeline");
    cyclicDef.addTask(TaskDefinition(TaskId("task_A"), "StepA", {TaskId("task_C")}));
    cyclicDef.addTask(TaskDefinition(TaskId("task_B"), "StepB", {TaskId("task_A")}));
    cyclicDef.addTask(TaskDefinition(TaskId("task_C"), "StepC", {TaskId("task_B")}));

    auto submitRes = scheduler.submit(cyclicDef);
    assert(!submitRes.isSuccess());
    assert(submitRes.error().find("Cycle detected") != std::string::npos);

    scheduler.shutdown();
    std::cout << "[PASSED] testCycleDetectionRejection (Cycle rejected upon submit)\n";
}

void testMissingDependencyRejection() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockTrackingExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 2);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    // B depends on non-existent X
    WorkflowDefinition missingDef(WorkflowId("missing_wf"), "MissingPipeline");
    missingDef.addTask(TaskDefinition(TaskId("task_A"), "StepA"));
    missingDef.addTask(TaskDefinition(TaskId("task_B"), "StepB", {TaskId("non_existent_X")}));

    auto submitRes = scheduler.submit(missingDef);
    assert(!submitRes.isSuccess());
    assert(submitRes.error().find("Missing dependency") != std::string::npos);

    scheduler.shutdown();
    std::cout << "[PASSED] testMissingDependencyRejection (Missing dependency rejected)\n";
}

void testTaskFailureWorkflowState() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockTrackingExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 2);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    WorkflowDefinition failDef(WorkflowId("fail_wf"), "FailPipeline");
    failDef.addTask(TaskDefinition(TaskId("task_crash"), "CrashTask"));

    auto submitRes = scheduler.submit(failDef);
    assert(submitRes.isSuccess());

    for (int i = 0; i < 50; ++i) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("fail_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Failed) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto execution = scheduler.getWorkflowExecution(WorkflowId("fail_wf"));
    assert(execution.has_value());
    assert(execution->state() == WorkflowState::Failed);
    assert(execution->getTaskExecution(TaskId("task_crash"))->state() == TaskState::Failed);

    scheduler.shutdown();
    std::cout << "[PASSED] testTaskFailureWorkflowState (Task failure transitions workflow to Failed)\n";
}

int main() {
    std::cout << "=== Running Milestone 7 Scheduler Layer Unit Tests ===\n";
    testDiamondWorkflowExecution();
    testCycleDetectionRejection();
    testMissingDependencyRejection();
    testTaskFailureWorkflowState();
    std::cout << "=== All Milestone 7 Unit Tests Passed Successfully! ===\n";
    return 0;
}
