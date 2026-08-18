#include <agentmesh/scheduler/PriorityReadyQueue.hpp>

namespace agentmesh::scheduler {

void PriorityReadyQueue::push(ReadyTask task) {
    std::lock_guard<std::mutex> lock(mutex_);
    queue_.push(QueueItem{
        .id = std::move(task.id),
        .priority = task.priority,
        .sequenceNumber = ++sequenceCounter_
    });
}

std::optional<domain::TaskId> PriorityReadyQueue::pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (queue_.empty()) {
        return std::nullopt;
    }
    auto item = queue_.top();
    queue_.pop();
    return item.id;
}

bool PriorityReadyQueue::empty() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

std::size_t PriorityReadyQueue::size() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

} // namespace agentmesh::scheduler
