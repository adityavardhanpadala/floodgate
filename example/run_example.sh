#!/bin/bash
set -e

# Create build directory if it doesn't exist
mkdir -p ../build
cd ../build

# Build the project
cmake ..
make

# Go back to example directory
cd ../example

# Compile the example to LLVM IR (without optnone attribute but with no inlining)
clang -emit-llvm -S -c -O0 example.c -o example.ll

# Run the pass
opt -load-pass-plugin=../build/lib/BacktracePass.so -passes=backtrace -S example.ll -o example.instrumented.ll

# Compile to object file
llc -filetype=obj example.instrumented.ll -o example.o

# Link with the runtime
clang example.o -o example_program -L../build/lib -lbacktrace_runtime -lunwind

# Run the instrumented program
echo "Running instrumented program:"
LD_LIBRARY_PATH=../build/lib ./example_program

