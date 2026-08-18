#include <agentmesh/graph/WorkflowGraph.hpp>

namespace agentmesh::graph {

WorkflowGraph::WorkflowGraph(const domain::WorkflowDefinition& workflowDef)
    : workflowDef_(workflowDef) {
    buildGraph();
}

void WorkflowGraph::buildGraph() {
    // Initialize nodes and in-degrees
    for (const auto& [id, task] : workflowDef_.tasks()) {
        nodes_.insert(id);
        inDegrees_[id] = 0;
        adjacencyList_[id] = {}; // Ensure entry exists
    }

    // Build edges (dependencies -> tasks)
    for (const auto& [id, task] : workflowDef_.tasks()) {
        for (const auto& dep : task.dependencies()) {
            adjacencyList_[dep].push_back(id);
            inDegrees_[id]++;
        }
    }
}

const std::unordered_set<domain::TaskId>& WorkflowGraph::getNodes() const noexcept {
    return nodes_;
}

const std::vector<domain::TaskId>& WorkflowGraph::getSuccessors(const domain::TaskId& id) const {
    auto it = adjacencyList_.find(id);
    if (it != adjacencyList_.end()) {
        return it->second;
    }
    static const std::vector<domain::TaskId> empty;
    return empty;
}

const std::unordered_map<domain::TaskId, int>& WorkflowGraph::getInDegrees() const noexcept {
    return inDegrees_;
}

int WorkflowGraph::getInDegree(const domain::TaskId& id) const {
    auto it = inDegrees_.find(id);
    if (it != inDegrees_.end()) {
        return it->second;
    }
    return 0;
}

const domain::WorkflowDefinition& WorkflowGraph::getWorkflowDefinition() const noexcept {
    return workflowDef_;
}

} // namespace agentmesh::graph