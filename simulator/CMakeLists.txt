cmake_minimum_required(VERSION 3.16)
project(ARCH16 C)
set(CMAKE_C_STANDARD 99)

# ‑‑‑‑‑ Where to drop the files (same dir that holds this CMakeLists.txt) ‑‑‑‑‑
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})   # executables
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR})   # *.so / *.dll

# include headers from sibling dirs
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}               # simulator headers
    ${CMAKE_CURRENT_LIST_DIR}/pipeline
    ${CMAKE_CURRENT_LIST_DIR}/../assembler  # …/assembler/*.h
)

# ---------------------------------------------------------------------------
#  Shared lib that only stores run‑time knobs
# ---------------------------------------------------------------------------
add_library(simconf SHARED
    ${CMAKE_CURRENT_LIST_DIR}/globals.c
)
set_target_properties(simconf PROPERTIES OUTPUT_NAME simconf)

# Copy the lib next to the Python GUI (…/gui/)
add_custom_command(TARGET simconf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            $<TARGET_FILE:simconf>
            ${CMAKE_CURRENT_LIST_DIR}/../gui
    COMMENT "Copying libsimconf to gui/ for ctypes")

# ---------------------------------------------------------------------------
#  Simulator executable
# ---------------------------------------------------------------------------
add_executable(simulator
    ${CMAKE_CURRENT_LIST_DIR}/simulator.c
    ${CMAKE_CURRENT_LIST_DIR}/memory.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline/fetch.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline/decode.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline/execute.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline/memory_access.c
    ${CMAKE_CURRENT_LIST_DIR}/pipeline/write_back.c
    ${CMAKE_CURRENT_LIST_DIR}/../assembler/assembler.c
)
target_link_libraries(simulator PRIVATE simconf)

# Optional warnings / optimisation
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(simulator PRIVATE -Wall -Wextra -O2)
    target_compile_options(simconf   PRIVATE -Wall -Wextra -O2)
endif()
