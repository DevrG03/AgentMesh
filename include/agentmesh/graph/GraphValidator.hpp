#ifndef AGENTMESH_GRAPH_GRAPH_VALIDATOR_HPP
#define AGENTMESH_GRAPH_GRAPH_VALIDATOR_HPP

#include <agentmesh/graph/WorkflowGraph.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::graph {

/// @brief Encapsulates validation logic for workflow DAGs.
class GraphValidator {
public:
    /// @brief Validates a workflow graph for missing dependencies and cycles.
    /// @return Success if valid, Error with details if invalid.
    static utils::Result<void> validate(const WorkflowGraph& graph);
};

} // namespace agentmesh::graph

#endif // AGENTMESH_GRAPH_GRAPH_VALIDATOR_HPP
