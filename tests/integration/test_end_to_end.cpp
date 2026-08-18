#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <agentmesh/scheduler/PriorityReadyQueue.hpp>
#include <agentmesh/execution/LocalWorkerPool.hpp>
#include <agentmesh/execution/ITaskExecutor.hpp>
#include <agentmesh/persistence/InMemoryStateRepository.hpp>
#include <agentmesh/scheduler/Scheduler.hpp>

using namespace agentmesh::domain;
using namespace agentmesh::scheduler;
using namespace agentmesh::execution;
using namespace agentmesh::persistence;

class MockE2EExecutor : public ITaskExecutor {
public:
    TaskResult execute(const TaskDefinition& task) override {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        return TaskResult::success("Executed: " + task.name());
    }
};

void testComplex20NodeDAG() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockE2EExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 8);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    // Build a 20-node multi-level diamond DAG:
    // Level 0: Roots [0, 1, 2, 3]
    // Level 1: [4, 5, 6, 7] -> dependent on Level 0
    // Level 2: [8, 9, 10, 11] -> dependent on Level 1
    // Level 3: [12, 13, 14, 15] -> dependent on Level 2
    // Level 4: [16, 17, 18] -> diamond join of Level 3
    // Level 5: [19] -> final terminal sink node
    WorkflowDefinition def(WorkflowId("complex_20_node_wf"), "Complex20NodePipeline");

    // Level 0 (Roots)
    for (int i = 0; i < 4; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "RootTask_" + std::to_string(i), {}, 10));
    }

    // Level 1
    for (int i = 4; i < 8; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "L1Task_" + std::to_string(i),
            {TaskId("t_" + std::to_string(i - 4))}, 8));
    }

    // Level 2
    for (int i = 8; i < 12; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "L2Task_" + std::to_string(i),
            {TaskId("t_" + std::to_string(i - 4))}, 6));
    }

    // Level 3
    for (int i = 12; i < 16; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "L3Task_" + std::to_string(i),
            {TaskId("t_" + std::to_string(i - 4))}, 4));
    }

    // Level 4 (Diamond Join)
    def.addTask(TaskDefinition(TaskId("t_16"), "L4Task_16", {TaskId("t_12"), TaskId("t_13")}, 2));
    def.addTask(TaskDefinition(TaskId("t_17"), "L4Task_17", {TaskId("t_13"), TaskId("t_14")}, 2));
    def.addTask(TaskDefinition(TaskId("t_18"), "L4Task_18", {TaskId("t_14"), TaskId("t_15")}, 2));

    // Level 5 (Final Sink)
    def.addTask(TaskDefinition(TaskId("t_19"), "FinalSinkTask", {TaskId("t_16"), TaskId("t_17"), TaskId("t_18")}, 1));

    assert(def.tasks().size() == 20);

    auto res = scheduler.submit(def);
    assert(res.isSuccess());

    // Wait for the 20 tasks to finish
    for (int i = 0; i < 200; ++i) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("complex_20_node_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    auto execution = scheduler.getWorkflowExecution(WorkflowId("complex_20_node_wf"));
    assert(execution.has_value());
    assert(execution->state() == WorkflowState::Completed);

    for (int i = 0; i < 20; ++i) {
        auto tExec = execution->getTaskExecution(TaskId("t_" + std::to_string(i)));
        assert(tExec.has_value());
        assert(tExec->state() == TaskState::Completed);
    }

    scheduler.shutdown();
    std::cout << "[PASSED] testComplex20NodeDAG (20-node multi-level DAG executed cleanly)\n";
}

void testMultiWorkflowConcurrentStress() {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<MockE2EExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 8);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    const int workflowCount = 5;
    for (int w = 0; w < workflowCount; ++w) {
        std::string prefix = "w" + std::to_string(w) + "_";
        WorkflowDefinition def(WorkflowId("concurrent_wf_" + std::to_string(w)), "ConcurrentDAG_" + std::to_string(w));
        def.addTask(TaskDefinition(TaskId(prefix + "t_a"), "TaskA", {}, 10));
        def.addTask(TaskDefinition(TaskId(prefix + "t_b"), "TaskB", {TaskId(prefix + "t_a")}, 5));
        def.addTask(TaskDefinition(TaskId(prefix + "t_c"), "TaskC", {TaskId(prefix + "t_b")}, 1));

        auto submitRes = scheduler.submit(def);
        assert(submitRes.isSuccess());
    }

    // Wait for all 5 concurrent workflows to finish
    for (int i = 0; i < 150; ++i) {
        bool allDone = true;
        for (int w = 0; w < workflowCount; ++w) {
            auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("concurrent_wf_" + std::to_string(w)));
            if (!wfOpt || wfOpt->state() != WorkflowState::Completed) {
                allDone = false;
                break;
            }
        }
        if (allDone) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    for (int w = 0; w < workflowCount; ++w) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("concurrent_wf_" + std::to_string(w)));
        assert(wfOpt.has_value());
        assert(wfOpt->state() == WorkflowState::Completed);
    }

    scheduler.shutdown();
    std::cout << "[PASSED] testMultiWorkflowConcurrentStress (5 concurrent workflows executed in parallel)\n";
}

int main() {
    std::cout << "=== Running Milestone 9 End-to-End Integration Suite ===\n";
    testComplex20NodeDAG();
    testMultiWorkflowConcurrentStress();
    std::cout << "=== All Milestone 9 End-to-End Tests Passed Successfully! ===\n";
    return 0;
}
