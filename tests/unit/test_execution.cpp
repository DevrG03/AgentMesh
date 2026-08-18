#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/TaskId.hpp>
#include <agentmesh/domain/TaskResult.hpp>
#include <agentmesh/execution/ITaskExecutor.hpp>
#include <agentmesh/execution/LocalWorkerPool.hpp>
#include <agentmesh/execution/ThreadPool.hpp>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <vector>

using namespace agentmesh::domain;
using namespace agentmesh::execution;

/// @brief Mock executor for normal tasks
class MockEchoExecutor : public ITaskExecutor {
public:
  TaskResult execute(const TaskDefinition &task) override {
    return TaskResult::success("Executed: " + task.name());
  }
};

/// @brief Mock executor that deliberately throws an exception
class MockThrowingExecutor : public ITaskExecutor {
public:
  TaskResult execute(const TaskDefinition &task) override {
    if (task.name() == "ThrowException") {
      throw std::runtime_error("Simulated crash!");
    }
    return TaskResult::success("OK");
  }
};

void testThreadPoolBasic() {
  ThreadPool pool(4);
  std::atomic<int> counter{0};
  const int totalJobs = 50;

  for (int i = 0; i < totalJobs; ++i) {
    pool.submit(
        [&counter]() { counter.fetch_add(1, std::memory_order_relaxed); });
  }

  pool.shutdown();
  assert(counter.load() == totalJobs);
  std::cout << "[PASSED] testThreadPoolBasic\n";
}

void testConcurrentDispatch() {
  auto executor = std::make_shared<MockEchoExecutor>();
  LocalWorkerPool pool(executor, 4);

  const int totalTasks = 100;
  std::atomic<int> completedTasks{0};
  std::mutex cvMutex;
  std::condition_variable cv;

  for (int i = 0; i < totalTasks; ++i) {
    TaskDefinition task(TaskId("task_" + std::to_string(i)),
                        "EchoTask_" + std::to_string(i));
    // FIX: Removed stray < and > around lambda arguments
    pool.dispatch(task, [&completedTasks, &cvMutex,
                         &cv](const TaskId &, const TaskResult & res) {
      assert(res.isSuccess());
      if (completedTasks.fetch_add(1) + 1 == totalTasks) {
        std::lock_guard<std::mutex> lock(cvMutex);
        cv.notify_one();
      }
    });
  }

  {
    std::unique_lock<std::mutex> lock(cvMutex);
    cv.wait(lock, [&completedTasks]() {
      return completedTasks.load() == totalTasks;
    });
  }

  assert(completedTasks.load() == totalTasks);
  pool.shutdown();
  std::cout << "[PASSED] testConcurrentDispatch (100 tasks executed "
               "asynchronously)\n";
}

void testExceptionSafety() {
  auto executor = std::make_shared<MockThrowingExecutor>();
  LocalWorkerPool pool(executor, 2);

  std::atomic<bool> failedTaskReported{false};
  std::atomic<bool> healthyTaskReported{false};
  std::mutex cvMutex;
  std::condition_variable cv;

  // 1. Dispatch a task that throws
  TaskDefinition badTask(TaskId("bad"), "ThrowException");
  // FIX: Removed stray < and > around lambda arguments
  pool.dispatch(badTask, [&failedTaskReported, &cvMutex,
                          &cv](const TaskId &, const TaskResult & res) {
    assert(!res.isSuccess());
    assert(res.error().find("Simulated crash!") != std::string::npos);
    failedTaskReported.store(true);
    std::lock_guard<std::mutex> lock(cvMutex);
    cv.notify_one();
  });

  // 2. Dispatch a healthy task afterwards (proves worker threads did not crash)
  TaskDefinition goodTask(TaskId("good"), "HealthyTask");
  // FIX: Removed stray < and > around lambda arguments
  pool.dispatch(goodTask, [&healthyTaskReported, &cvMutex,
                           &cv](const TaskId &, const TaskResult & res) {
    assert(res.isSuccess());
    healthyTaskReported.store(true);
    std::lock_guard<std::mutex> lock(cvMutex);
    cv.notify_one();
  });

  {
    std::unique_lock<std::mutex> lock(cvMutex);
    cv.wait(lock, [&failedTaskReported, &healthyTaskReported]() {
      return failedTaskReported.load() && healthyTaskReported.load();
    });
  }

  assert(failedTaskReported.load());
  assert(healthyTaskReported.load());
  pool.shutdown();
  std::cout
      << "[PASSED] testExceptionSafety (Workers survived throwing tasks)\n";
}

int main() {
  std::cout << "=== Running Milestone 5 Execution Layer Unit Tests ===\n";
  testThreadPoolBasic();
  testConcurrentDispatch();
  testExceptionSafety();
  std::cout << "=== All Milestone 5 Unit Tests Passed Successfully! ===\n";
  return 0;
}
