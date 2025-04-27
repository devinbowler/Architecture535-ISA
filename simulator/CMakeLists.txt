cmake_minimum_required(VERSION 3.16)
project(ARCH16 C)
set(CMAKE_C_STANDARD 99)

# ----- Include header files -----
include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/include
)

# ---------------------------------------------------------------------------
# Shared lib that only stores run‑time knobs
# ---------------------------------------------------------------------------
add_library(simconf SHARED
  ${CMAKE_CURRENT_LIST_DIR}/src/globals.c
)
set_target_properties(simconf PROPERTIES OUTPUT_NAME simconf)

# Copy the lib next to the Python GUI (…/gui/)
# Use file(COPY) to ensure it copies even if the file hasn't changed
add_custom_command(TARGET simconf POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/../gui
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:simconf>
    ${CMAKE_CURRENT_LIST_DIR}/../gui
  COMMENT "Copying libsimconf to gui/ for ctypes")

# Add a custom target to force copy
add_custom_target(force_copy_simconf ALL
  DEPENDS simconf
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:simconf>
    ${CMAKE_CURRENT_LIST_DIR}/../gui
  COMMENT "Force copying libsimconf to gui/ for ctypes")

# ----- simulator executable -----
add_executable(simulator
  ${CMAKE_CURRENT_LIST_DIR}/src/simulator.c
  ${CMAKE_CURRENT_LIST_DIR}/src/memory.c
  ${CMAKE_CURRENT_LIST_DIR}/src/hazards.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline/fetch.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline/decode.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline/execute.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline/memory_access.c
  ${CMAKE_CURRENT_LIST_DIR}/src/pipeline/write_back.c
  ${CMAKE_CURRENT_LIST_DIR}/src/assembler.c
)
target_link_libraries(simulator PRIVATE simconf)

# ----- Compiler flags -----
if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(simulator PRIVATE -Wall -Wextra -Wunused -O2)
  target_compile_options(simconf PRIVATE -Wall -Wextra -Wunused -O2)
endif()
