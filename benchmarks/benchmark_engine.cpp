#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <memory>
#include <cassert>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/WorkflowExecution.hpp>
#include <agentmesh/scheduler/PriorityReadyQueue.hpp>
#include <agentmesh/execution/LocalWorkerPool.hpp>
#include <agentmesh/execution/ITaskExecutor.hpp>
#include <agentmesh/persistence/InMemoryStateRepository.hpp>
#include <agentmesh/scheduler/Scheduler.hpp>

using namespace agentmesh::domain;
using namespace agentmesh::scheduler;
using namespace agentmesh::execution;
using namespace agentmesh::persistence;

// -----------------------------------------------------------------------------
// Benchmark Task Executors
// -----------------------------------------------------------------------------

/// @brief Zero-overhead executor isolating pure scheduler control-plane latency
class NoOpExecutor : public ITaskExecutor {
public:
    TaskResult execute([[maybe_unused]] const TaskDefinition& task) override {
        return TaskResult::success();
    }
};

/// @brief Controlled micro-workload executor for measuring multi-core speedup
class SyntheticWorkloadExecutor : public ITaskExecutor {
private:
    int iterations_;
public:
    explicit SyntheticWorkloadExecutor(int iterations = 2000) : iterations_(iterations) {}

    TaskResult execute([[maybe_unused]] const TaskDefinition& task) override {
        volatile double accumulator = 1.0;
        for (int i = 1; i <= iterations_; ++i) {
            accumulator = accumulator * 1.0001 + (i * 0.001);
        }
        return TaskResult::success();
    }
};

// -----------------------------------------------------------------------------
// Benchmark Suite Implementations
// -----------------------------------------------------------------------------

/// @brief Benchmark 1: Measure maximum task scheduling throughput with No-Op tasks
void benchmarkPureThroughput(int taskCount, unsigned int workerThreads) {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<NoOpExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, workerThreads);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    WorkflowDefinition def(WorkflowId("bench_throughput_wf"), "ThroughputPipeline");
    for (int i = 0; i < taskCount; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "NoOpTask", {}, i % 10));
    }

    const auto start = std::chrono::high_resolution_clock::now();
    auto submitRes = scheduler.submit(def);
    assert(submitRes.isSuccess());

    while (true) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("bench_throughput_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::yield();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const double tps = (static_cast<double>(taskCount) / elapsedMs) * 1000.0;

    std::cout << std::left << std::setw(38) << "1. Pure Task Scheduling Throughput"
              << std::right << std::setw(15) << (std::to_string(taskCount) + " tasks")
              << std::setw(15) << std::fixed << std::setprecision(2) << elapsedMs << " ms"
              << std::setw(15) << std::fixed << std::setprecision(0) << tps << " tasks/s\n";

    scheduler.shutdown();
}

/// @brief Benchmark 2: Measure inter-task unblock latency across a deep linear chain
void benchmarkSchedulingLag(int chainLength) {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<NoOpExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 4);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    WorkflowDefinition def(WorkflowId("bench_chain_wf"), "DeepLinearChain");
    def.addTask(TaskDefinition(TaskId("t_0"), "ChainRoot", {}, 5));

    for (int i = 1; i < chainLength; ++i) {
        def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "ChainTask_" + std::to_string(i),
            {TaskId("t_" + std::to_string(i - 1))}, 5));
    }

    const auto start = std::chrono::high_resolution_clock::now();
    auto submitRes = scheduler.submit(def);
    assert(submitRes.isSuccess());

    while (true) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("bench_chain_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::yield();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsedUs = std::chrono::duration<double, std::micro>(end - start).count();
    const double lagPerHopUs = elapsedUs / static_cast<double>(chainLength);

    std::cout << std::left << std::setw(38) << "2. Scheduling Lag (Chain Depth)"
              << std::right << std::setw(15) << (std::to_string(chainLength) + " hops")
              << std::setw(15) << std::fixed << std::setprecision(2) << (elapsedUs / 1000.0) << " ms"
              << std::setw(15) << std::fixed << std::setprecision(2) << lagPerHopUs << " μs / hop\n";

    scheduler.shutdown();
}

/// @brief Benchmark 3: Wide Fan-Out / Fan-In Map-Reduce Topology
void benchmarkWideFanOutFanIn(int parallelBranches, unsigned int workerThreads) {
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<NoOpExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, workerThreads);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(queue, dispatcher, repository);

    WorkflowDefinition def(WorkflowId("bench_fan_wf"), "FanOutFanInDAG");
    def.addTask(TaskDefinition(TaskId("root_task"), "Root", {}, 10));

    std::vector<TaskId> branchIds;
    branchIds.reserve(static_cast<std::size_t>(parallelBranches));

    for (int i = 0; i < parallelBranches; ++i) {
        TaskId branchId("branch_" + std::to_string(i));
        branchIds.push_back(branchId);
        def.addTask(TaskDefinition(branchId, "BranchTask", {TaskId("root_task")}, 5));
    }

    def.addTask(TaskDefinition(TaskId("sink_task"), "JoinSink", branchIds, 1));
    const int totalNodes = parallelBranches + 2;

    const auto start = std::chrono::high_resolution_clock::now();
    auto submitRes = scheduler.submit(def);
    assert(submitRes.isSuccess());

    while (true) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("bench_fan_wf"));
        if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
            break;
        }
        std::this_thread::yield();
    }

    const auto end = std::chrono::high_resolution_clock::now();
    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const double tps = (static_cast<double>(totalNodes) / elapsedMs) * 1000.0;

    std::cout << std::left << std::setw(38) << "3. Wide Fan-Out / Fan-In DAG"
              << std::right << std::setw(15) << (std::to_string(totalNodes) + " nodes")
              << std::setw(15) << std::fixed << std::setprecision(2) << elapsedMs << " ms"
              << std::setw(15) << std::fixed << std::setprecision(0) << tps << " tasks/s\n";

    scheduler.shutdown();
}

/// @brief Benchmark 4: Multi-Core Worker Scaling with Synthetic Workloads
void benchmarkMultiThreadScaling(int taskCount) {
    const std::vector<unsigned int> threadCounts = {1, 2, 4, 8};
    double singleThreadElapsedMs = 0.0;

    for (unsigned int threads : threadCounts) {
        auto queue = std::make_shared<PriorityReadyQueue>();
        auto executor = std::make_shared<SyntheticWorkloadExecutor>(3000);
        auto dispatcher = std::make_shared<LocalWorkerPool>(executor, threads);
        auto repository = std::make_shared<InMemoryStateRepository>();

        Scheduler scheduler(queue, dispatcher, repository);

        WorkflowDefinition def(WorkflowId("bench_scale_wf_" + std::to_string(threads)), "ScaleDAG");
        for (int i = 0; i < taskCount; ++i) {
            def.addTask(TaskDefinition(TaskId("t_" + std::to_string(i)), "ComputeTask", {}, 5));
        }

        const auto start = std::chrono::high_resolution_clock::now();
        auto submitRes = scheduler.submit(def);
        assert(submitRes.isSuccess());

        while (true) {
            auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("bench_scale_wf_" + std::to_string(threads)));
            if (wfOpt && wfOpt->state() == WorkflowState::Completed) {
                break;
            }
            std::this_thread::yield();
        }

        const auto end = std::chrono::high_resolution_clock::now();
        const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();

        if (threads == 1) {
            singleThreadElapsedMs = elapsedMs;
        }

        const double speedup = singleThreadElapsedMs / elapsedMs;

        std::string label = "4. Worker Scaling (" + std::to_string(threads) + " Thread" + (threads > 1 ? "s)" : ")");
        std::string speedupStr = "(" + std::to_string(speedup).substr(0, 4) + "x speedup)";

        std::cout << std::left << std::setw(38) << label
                  << std::right << std::setw(15) << (std::to_string(taskCount) + " tasks")
                  << std::setw(15) << std::fixed << std::setprecision(2) << elapsedMs << " ms"
                  << std::setw(15) << speedupStr << "\n";

        scheduler.shutdown();
    }
}

/// @brief Benchmark 5: Crash Recovery In-Memory State Reconstruction Throughput
void benchmarkCrashRecoveryThroughput(int workflowCount) {
    auto repository = std::make_shared<InMemoryStateRepository>();
    std::vector<WorkflowDefinition> definitions;
    definitions.reserve(static_cast<std::size_t>(workflowCount));

    // Pre-populate repository with 1,000 interrupted workflows
    for (int w = 0; w < workflowCount; ++w) {
        std::string prefix = "wf_" + std::to_string(w) + "_";
        WorkflowDefinition def(WorkflowId("recovery_wf_" + std::to_string(w)), "RecoveryDAG");
        def.addTask(TaskDefinition(TaskId(prefix + "A"), "A", {}, 10));
        def.addTask(TaskDefinition(TaskId(prefix + "B"), "B", {TaskId(prefix + "A")}, 5));
        def.addTask(TaskDefinition(TaskId(prefix + "C"), "C", {TaskId(prefix + "B")}, 1));

        WorkflowExecution execution(def);
        assert(execution.transitionTo(WorkflowState::Running).isSuccess());
        
        // Strict state transitions: Pending -> Ready -> Running -> Completed
        assert(execution.updateTaskState(TaskId(prefix + "A"), TaskState::Ready).isSuccess());
        assert(execution.updateTaskState(TaskId(prefix + "A"), TaskState::Running).isSuccess());
        assert(execution.updateTaskState(TaskId(prefix + "A"), TaskState::Completed).isSuccess());

        assert(execution.updateTaskState(TaskId(prefix + "B"), TaskState::Ready).isSuccess());
        assert(execution.updateTaskState(TaskId(prefix + "B"), TaskState::Running).isSuccess());

        repository->saveWorkflow(execution);
        definitions.push_back(def);
    }

    // Benchmark the exact time Scheduler::recover() takes to reconstruct all DAGs
    auto queue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<NoOpExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 8);

    Scheduler freshScheduler(queue, dispatcher, repository);

    const auto start = std::chrono::high_resolution_clock::now();
    auto recoverRes = freshScheduler.recover(definitions);
    const auto end = std::chrono::high_resolution_clock::now();

    assert(recoverRes.isSuccess());
    assert(static_cast<int>(recoverRes.value()) == workflowCount);

    const double elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
    const double recoveryRate = (static_cast<double>(workflowCount) / elapsedMs) * 1000.0;

    std::cout << std::left << std::setw(38) << "5. Crash Recovery Reconstruction"
              << std::right << std::setw(15) << (std::to_string(workflowCount) + " workflows")
              << std::setw(15) << std::fixed << std::setprecision(2) << elapsedMs << " ms"
              << std::setw(15) << std::fixed << std::setprecision(0) << recoveryRate << " wf / sec\n";

    freshScheduler.shutdown();
}

// -----------------------------------------------------------------------------
// Main Runner
// -----------------------------------------------------------------------------

int main() {
    const unsigned int hardwareCores = std::thread::hardware_concurrency();

    std::cout << "\n================================================================================\n";
    std::cout << "                 AgentMesh Phase 1 Performance Benchmark Suite                 \n";
    std::cout << "================================================================================\n";
    std::cout << "Hardware Concurrency : " << hardwareCores << " Logical Cores\n";
    std::cout << "Compiler             : Modern C++20 (Standard Optimization)\n";
    std::cout << "--------------------------------------------------------------------------------\n";
    std::cout << std::left << std::setw(38) << "Benchmark Suite"
              << std::right << std::setw(15) << "Workload Size"
              << std::setw(15) << "Total Time"
              << std::setw(15) << "Throughput / Lag\n";
    std::cout << "--------------------------------------------------------------------------------\n";

    benchmarkPureThroughput(10000, hardwareCores > 0 ? hardwareCores : 8);
    benchmarkSchedulingLag(1000);
    benchmarkWideFanOutFanIn(2000, hardwareCores > 0 ? hardwareCores : 8);
    benchmarkCrashRecoveryThroughput(1000);
    benchmarkMultiThreadScaling(2000);

    std::cout << "================================================================================\n\n";
    return 0;
}
