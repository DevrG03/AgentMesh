#ifndef AGENTMESH_GRAPH_WORKFLOW_GRAPH_HPP
#define AGENTMESH_GRAPH_WORKFLOW_GRAPH_HPP

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/WorkflowDefinition.hpp>

namespace agentmesh::graph {

/// @brief Represents the graph topology of a workflow.
class WorkflowGraph {
public:
    explicit WorkflowGraph(const domain::WorkflowDefinition& workflowDef);

    [[nodiscard]] const std::unordered_set<domain::TaskId>& getNodes() const noexcept;
    [[nodiscard]] const std::vector<domain::TaskId>& getSuccessors(const domain::TaskId& id) const;
    [[nodiscard]] const std::unordered_map<domain::TaskId, int>& getInDegrees() const noexcept;
    [[nodiscard]] int getInDegree(const domain::TaskId& id) const;
    
    [[nodiscard]] const domain::WorkflowDefinition& getWorkflowDefinition() const noexcept;

private:
    domain::WorkflowDefinition workflowDef_;
    std::unordered_set<domain::TaskId> nodes_;
    std::unordered_map<domain::TaskId, std::vector<domain::TaskId>> adjacencyList_;
    std::unordered_map<domain::TaskId, int> inDegrees_;

    void buildGraph();
};

} // namespace agentmesh::graph

#endif // AGENTMESH_GRAPH_WORKFLOW_GRAPH_HPP