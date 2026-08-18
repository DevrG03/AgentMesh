#ifndef AGENTMESH_DOMAIN_WORKFLOW_DEFINITION_HPP
#define AGENTMESH_DOMAIN_WORKFLOW_DEFINITION_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <utility>
#include <agentmesh/domain/WorkflowId.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/utils/Result.hpp>

namespace agentmesh::domain {

/// @brief Immutable specification of an entire DAG workflow.
class WorkflowDefinition {
public:
    WorkflowDefinition(WorkflowId id, std::string name)
        : id_(std::move(id)), name_(std::move(name)) {}

    [[nodiscard]] const WorkflowId& id() const noexcept { return id_; }
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    utils::Result<void> addTask(TaskDefinition task) {
        if (tasks_.contains(task.id())) {
            return utils::Result<void>::error("Duplicate task ID in workflow: " + task.id().value());
        }
        taskOrder_.push_back(task.id());
        tasks_.emplace(task.id(), std::move(task));
        return utils::Result<void>::success();
    }

    [[nodiscard]] bool hasTask(const TaskId& id) const noexcept {
        return tasks_.contains(id);
    }

    [[nodiscard]] std::optional<TaskDefinition> getTask(const TaskId& id) const {
        auto it = tasks_.find(id);
        if (it != tasks_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    [[nodiscard]] const std::unordered_map<TaskId, TaskDefinition>& tasks() const noexcept {
        return tasks_;
    }

    [[nodiscard]] const std::vector<TaskId>& taskOrder() const noexcept {
        return taskOrder_;
    }

private:
    WorkflowId id_;
    std::string name_;
    std::unordered_map<TaskId, TaskDefinition> tasks_;
    std::vector<TaskId> taskOrder_;
};

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_WORKFLOW_DEFINITION_HPP
