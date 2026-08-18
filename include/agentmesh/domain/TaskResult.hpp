#ifndef AGENTMESH_DOMAIN_TASK_RESULT_HPP
#define AGENTMESH_DOMAIN_TASK_RESULT_HPP

#include <string>
#include <chrono>
#include <utility>

namespace agentmesh::domain {

/// @brief Encapsulates the outcome of a single task execution.
class TaskResult {
public:
    TaskResult() = default;

    static TaskResult success(std::string output = "", std::chrono::milliseconds duration = std::chrono::milliseconds{0}) {
        TaskResult res;
        res.success_ = true;
        res.output_ = std::move(output);
        res.duration_ = duration;
        return res;
    }

    static TaskResult failure(std::string errorMsg, std::chrono::milliseconds duration = std::chrono::milliseconds{0}) {
        TaskResult res;
        res.success_ = false;
        res.error_ = std::move(errorMsg);
        res.duration_ = duration;
        return res;
    }

    [[nodiscard]] bool isSuccess() const noexcept { return success_; }
    [[nodiscard]] bool isFailure() const noexcept { return !success_; }
    [[nodiscard]] const std::string& output() const noexcept { return output_; }
    [[nodiscard]] const std::string& error() const noexcept { return error_; }
    [[nodiscard]] std::chrono::milliseconds duration() const noexcept { return duration_; }

private:
    bool success_{false};
    std::string output_;
    std::string error_;
    std::chrono::milliseconds duration_{0};
};

} // namespace agentmesh::domain

#endif // AGENTMESH_DOMAIN_TASK_RESULT_HPP
