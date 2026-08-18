#ifndef AGENTMESH_SCHEDULER_IREADY_QUEUE_HPP
#define AGENTMESH_SCHEDULER_IREADY_QUEUE_HPP

#include <optional>
#include <cstddef>
#include <agentmesh/domain/TaskId.hpp>

namespace agentmesh::scheduler {

/// @brief Value object representing a task waiting in the ready queue.
struct ReadyTask {
    domain::TaskId id;
    int priority{0};

    bool operator==(const ReadyTask&) const = default;
};

/// @brief Pure abstract interface for task ready queues (ISP & OCP).
class IReadyQueue {
public:
    virtual ~IReadyQueue() = default;

    /// @brief Pushes a ready task into the queue.
    virtual void push(ReadyTask task) = 0;

    /// @brief Pops the next highest priority task from the queue.
    /// @return The TaskId if available, std::nullopt if the queue is empty.
    virtual std::optional<domain::TaskId> pop() = 0;

    /// @brief Checks if the queue is empty.
    [[nodiscard]] virtual bool empty() const noexcept = 0;

    /// @brief Returns the number of tasks in the queue.
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
};

} // namespace agentmesh::scheduler

#endif // AGENTMESH_SCHEDULER_IREADY_QUEUE_HPP
