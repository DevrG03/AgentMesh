#ifndef AGENTMESH_SCHEDULER_PRIORITY_READY_QUEUE_HPP
#define AGENTMESH_SCHEDULER_PRIORITY_READY_QUEUE_HPP

#include <queue>
#include <mutex>
#include <cstdint>
#include <agentmesh/scheduler/IReadyQueue.hpp>

namespace agentmesh::scheduler {

/// @brief Thread-safe ready queue that schedules by Priority DESC with FIFO tie-breaking.
class PriorityReadyQueue : public IReadyQueue {
public:
    PriorityReadyQueue() = default;
    ~PriorityReadyQueue() override = default;

    // Non-copyable, non-movable to guarantee mutex thread safety
    PriorityReadyQueue(const PriorityReadyQueue&) = delete;
    PriorityReadyQueue& operator=(const PriorityReadyQueue&) = delete;
    PriorityReadyQueue(PriorityReadyQueue&&) = delete;
    PriorityReadyQueue& operator=(PriorityReadyQueue&&) = delete;

    void push(ReadyTask task) override;
    std::optional<domain::TaskId> pop() override;
    [[nodiscard]] bool empty() const noexcept override;
    [[nodiscard]] std::size_t size() const noexcept override;

private:
    struct QueueItem {
        domain::TaskId id;
        int priority;
        std::uint64_t sequenceNumber;

        // Custom comparator for std::priority_queue (Max-Heap):
        // 1. Highest priority comes first.
        // 2. On tie, smallest sequenceNumber (earliest arrival) comes first (FIFO).
        bool operator<(const QueueItem& other) const noexcept {
            if (priority != other.priority) {
                return priority < other.priority;
            }
            return sequenceNumber > other.sequenceNumber;
        }
    };

    mutable std::mutex mutex_;
    std::priority_queue<QueueItem> queue_;
    std::uint64_t sequenceCounter_{0};
};

} // namespace agentmesh::scheduler

#endif // AGENTMESH_SCHEDULER_PRIORITY_READY_QUEUE_HPP
