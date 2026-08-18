#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/graph/WorkflowGraph.hpp>
#include <agentmesh/graph/GraphValidator.hpp>
#include <iostream>
#include <cassert>

using namespace agentmesh::domain;
using namespace agentmesh::graph;

void testLinearGraph() {
    WorkflowDefinition wf(WorkflowId("wf_1"), "Linear");
    wf.addTask(TaskDefinition(TaskId("A"), "Task A"));
    wf.addTask(TaskDefinition(TaskId("B"), "Task B", {TaskId("A")}));
    wf.addTask(TaskDefinition(TaskId("C"), "Task C", {TaskId("B")}));

    WorkflowGraph graph(wf);
    auto result = GraphValidator::validate(graph);
    
    assert(result.isSuccess() && "Linear graph should pass validation");
    std::cout << "[PASSED] testLinearGraph\n";
}

void testDiamondGraph() {
    WorkflowDefinition wf(WorkflowId("wf_2"), "Diamond");
    wf.addTask(TaskDefinition(TaskId("A"), "Task A"));
    wf.addTask(TaskDefinition(TaskId("B"), "Task B", {TaskId("A")}));
    wf.addTask(TaskDefinition(TaskId("C"), "Task C", {TaskId("A")}));
    wf.addTask(TaskDefinition(TaskId("D"), "Task D", {TaskId("B"), TaskId("C")}));

    WorkflowGraph graph(wf);
    
    assert(graph.getInDegree(TaskId("A")) == 0);
    assert(graph.getInDegree(TaskId("B")) == 1);
    assert(graph.getInDegree(TaskId("C")) == 1);
    assert(graph.getInDegree(TaskId("D")) == 2);
    
    auto result = GraphValidator::validate(graph);
    
    assert(result.isSuccess() && "Diamond graph should pass validation");
    std::cout << "[PASSED] testDiamondGraph\n";
}

void testCycleGraph() {
    WorkflowDefinition wf(WorkflowId("wf_3"), "Cycle");
    wf.addTask(TaskDefinition(TaskId("A"), "Task A", {TaskId("C")})); // A -> C
    wf.addTask(TaskDefinition(TaskId("B"), "Task B", {TaskId("A")})); // B -> A
    wf.addTask(TaskDefinition(TaskId("C"), "Task C", {TaskId("B")})); // C -> B

    WorkflowGraph graph(wf);
    auto result = GraphValidator::validate(graph);
    
    assert(result.isError() && "Cycle graph should fail validation");
    assert(result.error() == "Cycle detected" && "Error message should be 'Cycle detected'");
    std::cout << "[PASSED] testCycleGraph\n";
}

void testMissingDependencyGraph() {
    WorkflowDefinition wf(WorkflowId("wf_4"), "Missing Dependency");
    wf.addTask(TaskDefinition(TaskId("A"), "Task A"));
    wf.addTask(TaskDefinition(TaskId("B"), "Task B", {TaskId("X")})); // depends on non-existent X

    WorkflowGraph graph(wf);
    auto result = GraphValidator::validate(graph);
    
    assert(result.isError() && "Missing dependency should fail validation");
    std::cout << "[PASSED] testMissingDependencyGraph\n";
}

int main() {
    std::cout << "=== Running Milestone 3 Graph Unit Tests ===\n";
    testLinearGraph();
    testDiamondGraph();
    testCycleGraph();
    testMissingDependencyGraph();
    std::cout << "=== All Milestone 3 Unit Tests Passed Successfully! ===\n";
    return 0;
}
