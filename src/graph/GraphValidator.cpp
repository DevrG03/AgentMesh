#include <agentmesh/graph/GraphValidator.hpp>
#include <queue>
#include <string>

namespace agentmesh::graph {

utils::Result<void> GraphValidator::validate(const WorkflowGraph& graph) {
    const auto& nodes = graph.getNodes();
    auto inDegrees = graph.getInDegrees();

    // 1. Check for missing dependencies
    for (const auto& [id, task] : graph.getWorkflowDefinition().tasks()) {
        for (const auto& dep : task.dependencies()) {
            if (!nodes.contains(dep)) {
                return utils::Result<void>::error(
                    "Missing dependency: Task " + id.value() + 
                    " depends on non-existent Task " + dep.value()
                );
            }
        }
    }

    // 2. Kahn's algorithm for cycle detection
    std::queue<domain::TaskId> zeroInDegreeNodes;
    for (const auto& [id, degree] : inDegrees) {
        if (degree == 0) {
            zeroInDegreeNodes.push(id);
        }
    }

    size_t visitedCount = 0;
    while (!zeroInDegreeNodes.empty()) {
        auto current = zeroInDegreeNodes.front();
        zeroInDegreeNodes.pop();
        visitedCount++;

        for (const auto& neighbor : graph.getSuccessors(current)) {
            if (--inDegrees[neighbor] == 0) {
                zeroInDegreeNodes.push(neighbor);
            }
        }
    }

    if (visitedCount != nodes.size()) {
        return utils::Result<void>::error("Cycle detected");
    }

    return utils::Result<void>::success();
}

} // namespace agentmesh::graph
