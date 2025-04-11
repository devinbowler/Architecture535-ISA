cmake_minimum_required(VERSION 3.10)
project(ProcessorSimulator LANGUAGES C CXX)

# Set default build type to Debug if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Find Python interpreter
find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
message(STATUS "Found Python: ${Python3_EXECUTABLE} (version ${Python3_VERSION})")

# Find Google Test package
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add simulator library with all simulator C source files
add_library(simulator
    simulator/simulator.c
    simulator/memory.c
    simulator/pipeline.c
    simulator/pipeline/execute.c
    simulator/pipeline/fetch.c
    simulator/pipeline/decode.c
    simulator/pipeline/memory_access.c
    simulator/pipeline/write_back.c
)

# Include directories for simulator
target_include_directories(simulator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add assembler library
add_library(assembler
    assembler/assembler.c
)

# Include directories for assembler
target_include_directories(assembler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add memory test executable
add_executable(memory_tests
    tests/MemoryTests.cpp
)

# Link the memory test executable
target_link_libraries(memory_tests
    simulator
    ${GTEST_LIBRARIES}
    pthread
)

# Add pipeline test executable
add_executable(pipeline_tests
    tests/PipelineTests.cpp
)

# Link the pipeline test executable
target_link_libraries(pipeline_tests
    simulator
    ${GTEST_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()

# Add tests to CTest
add_test(NAME MemoryTests COMMAND memory_tests)
add_test(NAME PipelineTests COMMAND pipeline_tests)

# Create a custom target to run all tests
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --verbose
    DEPENDS memory_tests pipeline_tests
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

# Python GUI handling
set(GUI_PYTHON_DIR ${CMAKE_SOURCE_DIR}/gui)

# Custom target for running the GUI
add_custom_target(run_gui
    COMMAND ${Python3_EXECUTABLE} ${GUI_PYTHON_DIR}/main.py
    WORKING_DIRECTORY ${GUI_PYTHON_DIR}
    COMMENT "Running Python GUI"
)

# Install Python requirements if there's a requirements.txt file
if(EXISTS ${GUI_PYTHON_DIR}/requirements.txt)
    add_custom_target(install_python_deps
        COMMAND ${Python3_EXECUTABLE} -m pip install -r requirements.txt
        WORKING_DIRECTORY ${GUI_PYTHON_DIR}
        COMMENT "Installing Python dependencies"
    )
endif()

# Optional: Add targets to create Python virtual environment
add_custom_target(create_venv
    COMMAND ${Python3_EXECUTABLE} -m venv ${CMAKE_BINARY_DIR}/venv
    COMMENT "Creating Python virtual environment"
)

# Add target to activate venv and install deps
if(EXISTS ${GUI_PYTHON_DIR}/requirements.txt)
    if(WIN32)
        add_custom_target(setup_python
            COMMAND ${CMAKE_COMMAND} -E echo "Run ${CMAKE_BINARY_DIR}/venv/Scripts/activate.bat manually, then run 'cmake --build . --target install_python_deps'"
            DEPENDS create_venv
        )
    else()
        add_custom_target(setup_python
            COMMAND ${CMAKE_COMMAND} -E echo "Run 'source ${CMAKE_BINARY_DIR}/venv/bin/activate' manually, then run 'cmake --build . --target install_python_deps'"
            DEPENDS create_venv
        )
    endif()
endif()
