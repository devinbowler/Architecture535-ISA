cmake_minimum_required(VERSION 3.10)
project(ProcessorSimulator LANGUAGES C CXX)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Find Google Test package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add simulator library with all simulator C source files
add_library(simulator_lib
    simulator/hazards.c
    simulator/memory.c
    simulator/pipeline.c
    simulator/pipeline/execute.c
    simulator/pipeline/fetch.c
    simulator/pipeline/decode.c
    simulator/pipeline/memory_access.c
    simulator/pipeline/write_back.c
    simulator/simulator.c
    assembler/assembler.c
    simulator/globals.c
)

# Include directories for simulator
target_include_directories(simulator_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add memory test executable
add_executable(memory_tests
    tests/MemoryTests.cpp
)

# Link the memory test executable
target_link_libraries(memory_tests
    simulator_lib
    ${GTEST_LIBRARIES}
    pthread
)

# Add pipeline test executable
add_executable(pipeline_tests
    tests/PipelineTests.cpp
)

# Link the pipeline test executable
target_link_libraries(pipeline_tests
    simulator_lib
    ${GTEST_LIBRARIES}
    pthread
)

add_executable(hazard_tests
    tests/HazardTests.cpp
)

target_link_libraries(hazard_tests
    simulator_lib
    ${GTEST_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME MemoryTests COMMAND memory_tests)
add_test(NAME PipelineTests COMMAND pipeline_tests)
add_test(NAME HazardTests COMMAND hazard_tests)

# Create a custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS memory_tests pipeline_tests hazard_tests
)

# Optional: Create individual test targets
add_custom_target(run_memory_tests
    COMMAND memory_tests --gtest_color=yes
    DEPENDS memory_tests
)

add_custom_target(run_pipeline_tests
    COMMAND pipeline_tests --gtest_color=yes
    DEPENDS pipeline_tests
)

add_custom_target(run_hazard_tests
    COMMAND hazard_tests --gtest_color=yes
    DEPENDS hazard_tests
)

add_executable(simulator
    simulator/simulator.c
    simulator/memory.c
    simulator/pipeline.c
    simulator/hazards.c
    simulator/pipeline/fetch.c
    simulator/pipeline/decode.c
    simulator/pipeline/execute.c
    simulator/pipeline/memory_access.c
    simulator/pipeline/write_back.c
    assembler/assembler.c
    simulator/globals.c
)