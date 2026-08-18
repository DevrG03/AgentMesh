#include <agentmesh/scheduler/PriorityReadyQueue.hpp>
#include <agentmesh/domain/TaskId.hpp>
#include <iostream>
#include <vector>
#include <thread>
#include <unordered_set>
#include <cassert>

using namespace agentmesh::domain;
using namespace agentmesh::scheduler;

void testPriorityOrdering() {
    PriorityReadyQueue queue;

    queue.push(ReadyTask{TaskId("low"), 1});
    queue.push(ReadyTask{TaskId("critical"), 100});
    queue.push(ReadyTask{TaskId("medium"), 50});

    assert(queue.size() == 3);
    assert(!queue.empty());

    assert(queue.pop() == TaskId("critical"));
    assert(queue.pop() == TaskId("medium"));
    assert(queue.pop() == TaskId("low"));
    assert(queue.pop() == std::nullopt);
    assert(queue.empty());

    std::cout << "[PASSED] testPriorityOrdering\n";
}

void testFifoTieBreaker() {
    PriorityReadyQueue queue;

    // Push tasks with identical priority
    queue.push(ReadyTask{TaskId("first"), 10});
    queue.push(ReadyTask{TaskId("second"), 10});
    queue.push(ReadyTask{TaskId("third"), 10});

    // Must pop in strict arrival order (FIFO)
    assert(queue.pop() == TaskId("first"));
    assert(queue.pop() == TaskId("second"));
    assert(queue.pop() == TaskId("third"));
    assert(queue.pop() == std::nullopt);

    std::cout << "[PASSED] testFifoTieBreaker\n";
}

void testConcurrentPushPop() {
    PriorityReadyQueue queue;
    const int numThreads = 8;
    const int tasksPerThread = 100;
    const int totalTasks = numThreads * tasksPerThread;

    std::vector<std::thread> producers;
    producers.reserve(numThreads);

    // 8 threads concurrently pushing tasks
    for (int t = 0; t < numThreads; ++t) {
        producers.emplace_back([&queue, t]() {
            for (int i = 0; i < tasksPerThread; ++i) {
                std::string id = "task_" + std::to_string(t) + "_" + std::to_string(i);
                queue.push(ReadyTask{TaskId(id), i % 10});
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    assert(queue.size() == static_cast<std::size_t>(totalTasks));

    // 8 threads concurrently popping tasks
    std::mutex resultMutex;
    std::unordered_set<std::string> poppedIds;
    std::vector<std::thread> consumers;
    consumers.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        consumers.emplace_back([&queue, &resultMutex, &poppedIds]() {
            while (true) {
                auto popped = queue.pop();
                if (!popped) {
                    break;
                }
                std::lock_guard<std::mutex> lock(resultMutex);
                poppedIds.insert(popped->value());
            }
        });
    }

    for (auto& t : consumers) {
        t.join();
    }

    assert(poppedIds.size() == static_cast<std::size_t>(totalTasks));
    assert(queue.empty());

    std::cout << "[PASSED] testConcurrentPushPop (8 threads, 800 tasks verified)\n";
}

int main() {
    std::cout << "=== Running Milestone 4 Ready Queue Unit Tests ===\n";
    testPriorityOrdering();
    testFifoTieBreaker();
    testConcurrentPushPop();
    std::cout << "=== All Milestone 4 Unit Tests Passed Successfully! ===\n";
    return 0;
}
