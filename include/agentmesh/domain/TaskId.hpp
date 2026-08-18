#ifndef AGENTMESH_DOMAIN_TASK_ID_HPP
#define AGENTMESH_DOMAIN_TASK_ID_HPP

#include <string>
#include <string_view>
#include <utility>
#include <functional>
#include <compare>

namespace agentmesh::domain {

/// @brief Strongly-typed value object representing a unique Task identifier.
class TaskId {
public:
    TaskId() = default;
    explicit TaskId(std::string id) : id_(std::move(id)) {}

    [[nodiscard]] const std::string& value() const noexcept { return id_; }
    [[nodiscard]] std::string_view view() const noexcept { return id_; }
    [[nodiscard]] bool empty() const noexcept { return id_.empty(); }

    auto operator<=>(const TaskId&) const = default;
    bool operator==(const TaskId&) const = default;

private:
    std::string id_;
};

} // namespace agentmesh::domain

namespace std {
template <>
struct hash<agentmesh::domain::TaskId> {
    size_t operator()(const agentmesh::domain::TaskId& id) const noexcept {
        return hash<std::string>{}(id.value());
    }
};
} // namespace std

#endif // AGENTMESH_DOMAIN_TASK_ID_HPP
