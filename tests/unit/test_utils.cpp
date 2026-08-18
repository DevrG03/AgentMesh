#include <agentmesh/utils/Result.hpp>
#include <agentmesh/utils/Synchronized.hpp>
#include <iostream>
#include <cassert>
#include <thread>
#include <vector>

void testResultSuccess() {
    auto res = agentmesh::utils::Result<int>::success(42);
    assert(res.isSuccess() == true);
    assert(res.isError() == false);
    assert(static_cast<bool>(res) == true);
    assert(res.value() == 42);
    assert(res.valueOr(100) == 42);
    std::cout << "[PASSED] testResultSuccess\n";
}

void testResultError() {
    auto res = agentmesh::utils::Result<int>::error("Invalid Operation");
    assert(res.isSuccess() == false);
    assert(res.isError() == true);
    assert(static_cast<bool>(res) == false);
    assert(res.error() == "Invalid Operation");
    assert(res.valueOr(100) == 100);
    std::cout << "[PASSED] testResultError\n";
}

void testSynchronizedConcurrency() {
    agentmesh::utils::Synchronized<int> counter(0);
    constexpr int kThreads = 10;
    constexpr int kIncrementsPerThread = 1000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < kIncrementsPerThread; ++j) {
                counter.withLock([](int& val) {
                    val++;
                });
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    int finalValue = counter.withLock([](const int& val) { return val; });
    assert(finalValue == kThreads * kIncrementsPerThread);
    std::cout << "[PASSED] testSynchronizedConcurrency (Final Counter: " << finalValue << ")\n";
}

int main() {
    std::cout << "=== Running Milestone 1 Utils Unit Tests ===\n";
    testResultSuccess();
    testResultError();
    testSynchronizedConcurrency();
    std::cout << "=== All Milestone 1 Unit Tests Passed Successfully! ===\n";
    return 0;
}
