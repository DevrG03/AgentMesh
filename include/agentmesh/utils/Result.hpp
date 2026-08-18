#ifndef AGENTMESH_UTILS_RESULT_HPP
#define AGENTMESH_UTILS_RESULT_HPP

#include <variant>
#include <string>
#include <utility>
#include <stdexcept>
#include <optional>

namespace agentmesh::utils {

/// @brief Lightweight monadic result type holding either a value of type T or an error of type E.
/// @tparam T The success value type.
/// @tparam E The error type (defaults to std::string).
template <typename T, typename E = std::string>
class Result {
public:
    // Factory method for success result
    static Result success(T value) {
        return Result(std::move(value));
    }

    // Factory method for error result
    static Result error(E err) {
        return Result(ErrorHolder{std::move(err)});
    }

    // Constructors
    /* implicit */ Result(T value) : data_(std::move(value)) {}

    // Query methods
    [[nodiscard]] bool isSuccess() const noexcept {
        return std::holds_alternative<T>(data_);
    }

    [[nodiscard]] bool isError() const noexcept {
        return std::holds_alternative<ErrorHolder>(data_);
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return isSuccess();
    }

    // Accessors
    [[nodiscard]] const T& value() const {
        if (isError()) {
            throw std::logic_error("Attempted to access value on an Error Result");
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] T& value() {
        if (isError()) {
            throw std::logic_error("Attempted to access value on an Error Result");
        }
        return std::get<T>(data_);
    }

    [[nodiscard]] const E& error() const {
        if (isSuccess()) {
            throw std::logic_error("Attempted to access error on a Success Result");
        }
        return std::get<ErrorHolder>(data_).error;
    }

    [[nodiscard]] T valueOr(T defaultValue) const {
        if (isSuccess()) {
            return std::get<T>(data_);
        }
        return defaultValue;
    }

private:
    struct ErrorHolder {
        E error;
    };

    explicit Result(ErrorHolder errHolder) : data_(std::move(errHolder)) {}

    std::variant<T, ErrorHolder> data_;
};

/// @brief Specialization for void return types.
template <typename E>
class Result<void, E> {
public:
    static Result success() {
        return Result();
    }

    static Result error(E err) {
        return Result(std::move(err));
    }

    [[nodiscard]] bool isSuccess() const noexcept {
        return !error_.has_value();
    }

    [[nodiscard]] bool isError() const noexcept {
        return error_.has_value();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return isSuccess();
    }

    [[nodiscard]] const E& error() const {
        if (isSuccess()) {
            throw std::logic_error("Attempted to access error on a Success Result");
        }
        return *error_;
    }

private:
    Result() = default;
    explicit Result(E err) : error_(std::move(err)) {}

    std::optional<E> error_;
};

} // namespace agentmesh::utils

#endif // AGENTMESH_UTILS_RESULT_HPP
