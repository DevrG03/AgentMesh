cmake_minimum_required(VERSION 3.20)
project(AgentMeshPhase1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler Warning Flags
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Set Output Directory for Test Executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Include Directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Header-only Utils Library
add_library(agentmesh_utils INTERFACE)
target_include_directories(agentmesh_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Domain Layer Library
add_library(agentmesh_domain STATIC
    src/domain/TaskExecution.cpp
    src/domain/WorkflowExecution.cpp
)
target_link_libraries(agentmesh_domain PUBLIC agentmesh_utils)
target_include_directories(agentmesh_domain PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Graph Layer Library (Milestone 3)
add_library(agentmesh_graph STATIC
    src/graph/WorkflowGraph.cpp
    src/graph/GraphValidator.cpp
)
target_link_libraries(agentmesh_graph PUBLIC agentmesh_domain)
target_include_directories(agentmesh_graph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Scheduler Layer Library (Milestone 4 & 7)
add_library(agentmesh_scheduler STATIC
    src/scheduler/PriorityReadyQueue.cpp
    src/scheduler/Scheduler.cpp
)
target_link_libraries(agentmesh_scheduler PUBLIC agentmesh_domain agentmesh_graph agentmesh_execution agentmesh_persistence agentmesh_utils)
target_include_directories(agentmesh_scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Execution Layer Library (Milestone 5)
add_library(agentmesh_execution STATIC
    src/execution/ThreadPool.cpp
    src/execution/LocalWorkerPool.cpp
)
target_link_libraries(agentmesh_execution PUBLIC agentmesh_domain agentmesh_utils)
target_include_directories(agentmesh_execution PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Persistence Layer Library (Milestone 6)
add_library(agentmesh_persistence STATIC
    src/persistence/InMemoryStateRepository.cpp
)
target_link_libraries(agentmesh_persistence PUBLIC agentmesh_domain agentmesh_utils)
target_include_directories(agentmesh_persistence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Main Application Executable (Milestone 9 - Composition Root)
add_executable(agentmesh_app apps/main.cpp)
set_target_properties(agentmesh_app PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apps")
target_link_libraries(agentmesh_app PRIVATE agentmesh_scheduler agentmesh_execution agentmesh_persistence agentmesh_graph agentmesh_domain agentmesh_utils)

# Enable Testing
enable_testing()

# Unit Test: Utils (Milestone 1)
add_executable(test_utils tests/unit/test_utils.cpp)
set_target_properties(test_utils PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_utils PRIVATE agentmesh_utils)
add_test(NAME test_utils COMMAND test_utils)

# Unit Test: Domain (Milestone 2)
add_executable(test_domain tests/unit/test_domain.cpp)
set_target_properties(test_domain PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_domain PRIVATE agentmesh_domain)
add_test(NAME test_domain COMMAND test_domain)

# Unit Test: Graph (Milestone 3)
add_executable(test_graph tests/unit/test_graph.cpp)
set_target_properties(test_graph PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_graph PRIVATE agentmesh_graph agentmesh_domain)
add_test(NAME test_graph COMMAND test_graph)

# Unit Test: Ready Queue (Milestone 4)
add_executable(test_ready_queue tests/unit/test_ready_queue.cpp)
set_target_properties(test_ready_queue PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_ready_queue PRIVATE agentmesh_scheduler agentmesh_domain)
add_test(NAME test_ready_queue COMMAND test_ready_queue)

# Unit Test: Execution (Milestone 5)
add_executable(test_execution tests/unit/test_execution.cpp)
set_target_properties(test_execution PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_execution PRIVATE agentmesh_execution agentmesh_domain)
add_test(NAME test_execution COMMAND test_execution)

# Unit Test: Persistence (Milestone 6)
add_executable(test_persistence tests/unit/test_persistence.cpp)
set_target_properties(test_persistence PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_persistence PRIVATE agentmesh_persistence agentmesh_domain)
add_test(NAME test_persistence COMMAND test_persistence)

# Unit Test: Scheduler (Milestone 7)
add_executable(test_scheduler tests/unit/test_scheduler.cpp)
set_target_properties(test_scheduler PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_scheduler PRIVATE agentmesh_scheduler agentmesh_execution agentmesh_persistence agentmesh_graph agentmesh_domain)
add_test(NAME test_scheduler COMMAND test_scheduler)

# Integration Test: Restart Recovery (Milestone 8)
add_executable(test_restart_recovery tests/integration/test_restart_recovery.cpp)
set_target_properties(test_restart_recovery PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_restart_recovery PRIVATE agentmesh_scheduler agentmesh_execution agentmesh_persistence agentmesh_graph agentmesh_domain)
add_test(NAME test_restart_recovery COMMAND test_restart_recovery)

# End-to-End Integration Test (Milestone 9)
add_executable(test_end_to_end tests/integration/test_end_to_end.cpp)
set_target_properties(test_end_to_end PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
target_link_libraries(test_end_to_end PRIVATE agentmesh_scheduler agentmesh_execution agentmesh_persistence agentmesh_graph agentmesh_domain)
add_test(NAME test_end_to_end COMMAND test_end_to_end)
