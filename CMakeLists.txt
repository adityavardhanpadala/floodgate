cmake_minimum_required(VERSION 3.10)
project(floodgate)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Build the LLVM pass
add_library(BacktracePass MODULE
  src/passes/BacktracePass.cpp
)

# Set C++14 standard (required for LLVM)
target_compile_features(BacktracePass PRIVATE cxx_std_14)

# Allow undefined symbols in the pass (we'll link with the runtime)
set_target_properties(BacktracePass PROPERTIES
  COMPILE_FLAGS "-fno-rtti"
  PREFIX ""
)

# Build the runtime library
add_library(backtrace_runtime SHARED
  src/runtime/backtrace.c
)

# Set output directories
set_target_properties(BacktracePass PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set_target_properties(backtrace_runtime PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Link against libunwind
target_link_libraries(backtrace_runtime "-lunwind")