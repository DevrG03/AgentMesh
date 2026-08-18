#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/domain/TaskState.hpp>
#include <agentmesh/domain/WorkflowState.hpp>
#include <agentmesh/domain/TaskResult.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskExecution.hpp>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <iostream>
#include <cassert>

using namespace agentmesh::domain;

void testValueObjects() {
    TaskId t1("task-1");
    TaskId t2("task-1");
    TaskId t3("task-2");

    assert(t1 == t2);
    assert(t1 != t3);
    assert(t1.value() == "task-1");

    WorkflowId w1("wf-1");
    assert(w1.value() == "wf-1");
    std::cout << "[PASSED] testValueObjects\n";
}

void testTaskStateTransitions() {
    TaskExecution task(TaskId("task-A"));
    assert(task.state() == TaskState::Pending);

    // Valid transition: Pending -> Ready
    auto res1 = task.transitionTo(TaskState::Ready);
    assert(res1.isSuccess());
    assert(task.state() == TaskState::Ready);

    // Invalid transition: Ready -> Completed (must go through Running)
    auto res2 = task.transitionTo(TaskState::Completed);
    assert(res2.isError());
    assert(task.state() == TaskState::Ready);

    // Valid transition: Ready -> Running
    task.markStarted();
    assert(task.state() == TaskState::Running);
    assert(task.attempt() == 1);

    // Valid transition: Running -> Completed
    task.markCompleted(TaskResult::success("output data"));
    assert(task.state() == TaskState::Completed);
    assert(task.result().has_value());
    assert(task.result()->output() == "output data");

    // Invalid transition: Completed -> Ready
    auto res3 = task.transitionTo(TaskState::Ready);
    assert(res3.isError());
    std::cout << "[PASSED] testTaskStateTransitions\n";
}

void testWorkflowDefinitionAndExecution() {
    WorkflowDefinition wfDef(WorkflowId("wf-main"), "Data Pipeline");
    
    TaskDefinition taskA(TaskId("A"), "Task A", {}, 10, "payloadA");
    TaskDefinition taskB(TaskId("B"), "Task B", {TaskId("A")}, 5, "payloadB");

    assert(wfDef.addTask(taskA).isSuccess());
    assert(wfDef.addTask(taskB).isSuccess());
    assert(wfDef.addTask(taskA).isError()); // Duplicate rejection

    WorkflowExecution wfExec(wfDef);
    assert(wfExec.state() == WorkflowState::Pending);
    assert(wfExec.hasTask(TaskId("A")));
    assert(wfExec.hasTask(TaskId("B")));

    // Workflow transition: Pending -> Running
    assert(wfExec.transitionTo(WorkflowState::Running).isSuccess());

    // Update Task A state
    assert(wfExec.updateTaskState(TaskId("A"), TaskState::Ready).isSuccess());
    auto taskAExec = wfExec.getTaskExecution(TaskId("A"));
    assert(taskAExec.has_value());
    assert(taskAExec->state() == TaskState::Ready);

    // Workflow transition: Running -> Completed
    assert(wfExec.transitionTo(WorkflowState::Completed).isSuccess());
    std::cout << "[PASSED] testWorkflowDefinitionAndExecution\n";
}

int main() {
    std::cout << "=== Running Milestone 2 Domain Unit Tests ===\n";
    testValueObjects();
    testTaskStateTransitions();
    testWorkflowDefinitionAndExecution();
    std::cout << "=== All Milestone 2 Unit Tests Passed Successfully! ===\n";
    return 0;
}
