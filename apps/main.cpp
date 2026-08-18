#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <iomanip>
#include <agentmesh/domain/WorkflowDefinition.hpp>
#include <agentmesh/domain/TaskDefinition.hpp>
#include <agentmesh/domain/TaskResult.hpp>
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

/// @brief Realistic logging task executor simulating discrete compute workloads
class PipelineTaskExecutor : public ITaskExecutor {
public:
    TaskResult execute(const TaskDefinition& task) override {
        std::cout << "  [Worker] Executing: " << std::setw(22) << std::left << task.name()
                  << " (Priority: " << std::setw(2) << task.priority() << ")\n";

        // Simulate varying processing durations
        std::this_thread::sleep_for(std::chrono::milliseconds(30));

        return TaskResult::success("Finished: " + task.name());
    }
};

int main() {
    std::cout << "====================================================\n";
    std::cout << "       AgentMesh Workflow Orchestrator (Phase 1)    \n";
    std::cout << "====================================================\n\n";

    // 1. Composition Root: Wire concrete implementations (DIP)
    auto readyQueue = std::make_shared<PriorityReadyQueue>();
    auto executor = std::make_shared<PipelineTaskExecutor>();
    auto dispatcher = std::make_shared<LocalWorkerPool>(executor, 4);
    auto repository = std::make_shared<InMemoryStateRepository>();

    Scheduler scheduler(readyQueue, dispatcher, repository);

    // 2. Define an End-to-End Machine Learning Pipeline DAG
    //
    //              [fetch_data_A]        [fetch_data_B]
    //                    │                      │
    //                    ▼                      ▼
    //              [clean_data_A]        [clean_data_B]
    //                    │                      │
    //                    ▼                      ▼
    //              [extract_feat_A]      [extract_feat_B]
    //                    \                      /
    //                     ▼                    ▼
    //                      [merge_and_normalize]
    //                                │
    //                                ▼
    //                          [train_model]
    //                                │
    //                                ▼
    //                        [evaluate_metrics]
    //                                │
    //                                ▼
    //                        [publish_report]
    //
    WorkflowDefinition mlPipeline(WorkflowId("ml_pipeline_v1"), "MLOpsDataProcessingDAG");

    // Stage 1: Data Fetching (Parallel Roots, High Priority = 10)
    mlPipeline.addTask(TaskDefinition(TaskId("fetch_data_A"), "FetchRawDatasetA", {}, 10));
    mlPipeline.addTask(TaskDefinition(TaskId("fetch_data_B"), "FetchRawDatasetB", {}, 10));

    // Stage 2: Data Cleaning (Priority = 8)
    mlPipeline.addTask(TaskDefinition(TaskId("clean_data_A"), "CleanDatasetA", {TaskId("fetch_data_A")}, 8));
    mlPipeline.addTask(TaskDefinition(TaskId("clean_data_B"), "CleanDatasetB", {TaskId("fetch_data_B")}, 8));

    // Stage 3: Feature Extraction (Priority = 6)
    mlPipeline.addTask(TaskDefinition(TaskId("extract_feat_A"), "ExtractFeaturesA", {TaskId("clean_data_A")}, 6));
    mlPipeline.addTask(TaskDefinition(TaskId("extract_feat_B"), "ExtractFeaturesB", {TaskId("clean_data_B")}, 6));

    // Stage 4: Merge & Normalize (Diamond Join)
    mlPipeline.addTask(TaskDefinition(TaskId("merge_data"), "MergeAndNormalize", {TaskId("extract_feat_A"), TaskId("extract_feat_B")}, 5));

    // Stage 5: Training & Evaluation
    mlPipeline.addTask(TaskDefinition(TaskId("train_model"), "TrainGradientBoostModel", {TaskId("merge_data")}, 4));
    mlPipeline.addTask(TaskDefinition(TaskId("evaluate"), "EvaluateAccuracyAndF1", {TaskId("train_model")}, 3));
    mlPipeline.addTask(TaskDefinition(TaskId("publish"), "PublishModelArtifacts", {TaskId("evaluate")}, 1));

    std::cout << "[Main] Submitting workflow: " << mlPipeline.name()
              << " (" << mlPipeline.tasks().size() << " tasks)\n";

    const auto startTime = std::chrono::steady_clock::now();
    auto submitResult = scheduler.submit(mlPipeline);

    if (!submitResult.isSuccess()) {
        std::cerr << "[Main Error] Submission failed: " << submitResult.error() << "\n";
        return 1;
    }

    std::cout << "[Main] Workflow submitted successfully. Executing DAG across 4 worker threads...\n\n";

    // 3. Monitor execution progress
    while (true) {
        auto wfOpt = scheduler.getWorkflowExecution(WorkflowId("ml_pipeline_v1"));
        if (wfOpt) {
            if (wfOpt->state() == WorkflowState::Completed || wfOpt->state() == WorkflowState::Failed) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    const auto endTime = std::chrono::steady_clock::now();
    const auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    // 4. Print Final State Summary from StateRepository
    auto finalWf = scheduler.getWorkflowExecution(WorkflowId("ml_pipeline_v1"));
    std::cout << "\n====================================================\n";
    std::cout << "                 Execution Summary                  \n";
    std::cout << "====================================================\n";
    std::cout << "Workflow ID     : " << finalWf->id().value() << "\n";
    std::cout << "Final State     : " << toString(finalWf->state()) << "\n";
    std::cout << "Total Elapsed   : " << totalDuration << " ms\n";
    std::cout << "Task Breakdown  :\n";

    for (const auto& taskId : mlPipeline.taskOrder()) {
        auto taskExec = finalWf->getTaskExecution(taskId);
        std::cout << "  • " << std::setw(20) << std::left << taskId.value()
                  << " -> State: " << toString(taskExec->state()) << "\n";
    }

    std::cout << "====================================================\n";

    // 5. Clean Shutdown
    scheduler.shutdown();
    std::cout << "[Main] Engine shutdown cleanly.\n";
    return 0;
}
