cmake_minimum_required(VERSION 3.10)

# Project name
project(ARCH-16)

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O2")

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
    )

# Add executable target
add_executable(ARCH-16 ${SOURCES})
