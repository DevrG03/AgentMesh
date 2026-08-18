#ifndef AGENTMESH_DOMAIN_TASK_DEFINITION_HPP
#define AGENTMESH_DOMAIN_TASK_DEFINITION_HPP

#include <string>
#include <vector>
#include <utility>
#include <agentmesh/domain/TaskId.hpp>

namespace agentmesh::domain {

/// @brief Immutable specification of a task within a workflow definition.
class TaskDefinition {
public:
    TaskDefinition(
        TaskId id,
        std::string name,
        std::vector<TaskId> dependencies = {},
        int priority = 0,
        std::string payload = ""
    ) : id_(std::move(id)),
        name_(std::move(name)),
        dependencies_(std::move(dependencies)),
        priority_(priority),
        payload_(std::move(payload)) {}

    [[nodiscard]] const TaskId& id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::vector<TaskId>& dependencies() const noexcept { return dependencies_; }
    [[nodiscard]] int priority() const noexcept { return priority_; }
    [[nodiscard]] const std::string& payload() const noexcept { return payload_; }

private:
    TaskId id_;
    std::string name_;
    std::vector<TaskId> dependencies_;
    int priority_{0};
    std::string payload_;
};

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_TASK_DEFINITION_HPP
