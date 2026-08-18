#ifndef AGENTMESH_UTILS_SYNCHRONIZED_HPP
#define AGENTMESH_UTILS_SYNCHRONIZED_HPP

#include <mutex>
#include <utility>

namespace agentmesh::utils {

/// @brief Thread-safe wrapper encapsulating data and a mutex (Monitor Pattern).
/// @tparam T The type of object protected by synchronization.
template <typename T>
class Synchronized {
public:
    Synchronized() = default;

    explicit Synchronized(T initialValue) : value_(std::move(initialValue)) {}

    // Disable copy to prevent accidental shallow/deep copy of locked states
    Synchronized(const Synchronized&) = delete;
    Synchronized& operator=(const Synchronized&) = delete;

    // Move operations
    Synchronized(Synchronized&& other) noexcept {
        std::lock_guard lock(other.mutex_);
        value_ = std::move(other.value_);
    }

    Synchronized& operator=(Synchronized&& other) noexcept {
        if (this != &other) {
            std::scoped_lock lock(mutex_, other.mutex_);
            value_ = std::move(other.value_);
        }
        return *this;
    }

    /// @brief Executes a lambda/callable while holding the lock.
    /// @tparam F Callable type accepting `T&`.
    template <typename F>
    auto withLock(F&& func) {
        std::lock_guard lock(mutex_);
        return func(value_);
    }

    /// @brief Executes a lambda/callable while holding the lock (const access).
    /// @tparam F Callable type accepting `const T&`.
    template <typename F>
    auto withLock(F&& func) const {
        std::lock_guard lock(mutex_);
        return func(value_);
    }

private:
    mutable std::mutex mutex_;
    T value_;
};

} // namespace agentmesh::utils

#endif // AGENTMESH_UTILS_SYNCHRONIZED_HPP
