cmake_minimum_required(VERSION 3.10)

# Project name
project(ARCH-16)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

# Set output directory to simulator root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Source files
set(SOURCES 
    simulator.c 
    memory.c 
    pipeline.c 
    pipeline/fetch.c 
    pipeline/decode.c 
    pipeline/execute.c 
    pipeline/memory_access.c 
    pipeline/write_back.c
    ../gui/api.py
    ../gui/interface.py
    ../assembler/assembler.c
    )

# Add executable target
add_executable(simulator ${SOURCES})
