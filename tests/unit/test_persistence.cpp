#include <iostream>
#include <cassert>
#include <vector>
#include <thread>
#include <agentmesh/persistence/InMemoryStateRepository.hpp>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>

using namespace agentmesh::domain;
using namespace agentmesh::persistence;

void testSaveAndLoadWorkflow() {
    InMemoryStateRepository repo;

    WorkflowDefinition def(WorkflowId("wf_1"), "TestPipeline");
    def.addTask(TaskDefinition(TaskId("task_A"), "StepA"));
    def.addTask(TaskDefinition(TaskId("task_B"), "StepB"));

    WorkflowExecution execution(def);
    repo.saveWorkflow(execution);

    assert(repo.size() == 1);

    auto loaded = repo.loadWorkflow(WorkflowId("wf_1"));
    assert(loaded.has_value());
    assert(loaded->id() == WorkflowId("wf_1"));
    assert(loaded->state() == WorkflowState::Pending);
    assert(loaded->hasTask(TaskId("task_A")));
    assert(loaded->hasTask(TaskId("task_B")));

    // Non-existent workflow
    auto missing = repo.loadWorkflow(WorkflowId("non_existent"));
    assert(!missing.has_value());

    std::cout << "[PASSED] testSaveAndLoadWorkflow\n";
}

void testUpdateTaskState() {
    InMemoryStateRepository repo;

    WorkflowDefinition def(WorkflowId("wf_1"), "TestPipeline");
    def.addTask(TaskDefinition(TaskId("task_A"), "StepA"));
    WorkflowExecution execution(def);
    repo.saveWorkflow(execution);

    // 1. Valid transition: Pending -> Ready
    auto res1 = repo.updateTaskState(WorkflowId("wf_1"), TaskId("task_A"), TaskState::Ready);
    assert(res1.isSuccess());

    auto loaded = repo.loadWorkflow(WorkflowId("wf_1"));
    assert(loaded.has_value());
    auto taskA = loaded->getTaskExecution(TaskId("task_A"));
    assert(taskA.has_value());
    assert(taskA->state() == TaskState::Ready);

    // 2. Invalid transition: Ready -> Completed (must go through Running)
    auto res2 = repo.updateTaskState(WorkflowId("wf_1"), TaskId("task_A"), TaskState::Completed);
    assert(!res2.isSuccess());

    // 3. Non-existent task
    auto res3 = repo.updateTaskState(WorkflowId("wf_1"), TaskId("ghost_task"), TaskState::Ready);
    assert(!res3.isSuccess());

    // 4. Non-existent workflow
    auto res4 = repo.updateTaskState(WorkflowId("ghost_wf"), TaskId("task_A"), TaskState::Ready);
    assert(!res4.isSuccess());

    std::cout << "[PASSED] testUpdateTaskState\n";
}

void testLoadActiveWorkflows() {
    InMemoryStateRepository repo;

    WorkflowExecution wf1(WorkflowId("wf_pending"), WorkflowState::Pending);
    WorkflowExecution wf2(WorkflowId("wf_running"), WorkflowState::Running);
    WorkflowExecution wf3(WorkflowId("wf_completed"), WorkflowState::Completed);
    WorkflowExecution wf4(WorkflowId("wf_failed"), WorkflowState::Failed);

    repo.saveWorkflow(wf1);
    repo.saveWorkflow(wf2);
    repo.saveWorkflow(wf3);
    repo.saveWorkflow(wf4);

    assert(repo.size() == 4);

    auto activeWorkflows = repo.loadActiveWorkflows();
    assert(activeWorkflows.size() == 2);

    bool foundPending = false;
    bool foundRunning = false;
    for (const auto& wf : activeWorkflows) {
        if (wf.id() == WorkflowId("wf_pending")) foundPending = true;
        if (wf.id() == WorkflowId("wf_running")) foundRunning = true;
    }
    assert(foundPending && foundRunning);

    std::cout << "[PASSED] testLoadActiveWorkflows\n";
}

void testConcurrentRepositoryAccess() {
    InMemoryStateRepository repo;
    const int numThreads = 8;
    const int opsPerThread = 50;

    // Seed repository
    for (int t = 0; t < numThreads; ++t) {
        WorkflowDefinition def(WorkflowId("wf_" + std::to_string(t)), "Pipeline_" + std::to_string(t));
        for (int i = 0; i < 5; ++i) {
            def.addTask(TaskDefinition(TaskId("task_" + std::to_string(i)), "SubTask"));
        }
        repo.saveWorkflow(WorkflowExecution(def));
    }

    std::vector<std::thread> workers;
    workers.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&repo, t]() {
            WorkflowId wfId("wf_" + std::to_string(t));
            for (int i = 0; i < opsPerThread; ++i) {
                // Read operations
                auto loaded = repo.loadWorkflow(wfId);
                assert(loaded.has_value());
                auto active = repo.loadActiveWorkflows();
                assert(!active.empty());

                // Update operation
                repo.updateTaskState(wfId, TaskId("task_0"), TaskState::Ready);
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    std::cout << "[PASSED] testConcurrentRepositoryAccess (8 threads, 400 operations verified)\n";
}

int main() {
    std::cout << "=== Running Milestone 6 Persistence Layer Unit Tests ===\n";
    testSaveAndLoadWorkflow();
    testUpdateTaskState();
    testLoadActiveWorkflows();
    testConcurrentRepositoryAccess();
    std::cout << "=== All Milestone 6 Unit Tests Passed Successfully! ===\n";
    return 0;
}
