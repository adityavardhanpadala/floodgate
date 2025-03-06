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
clang example.o -o example_program -L../build/lib -lbacktrace_runtime -lunwind -lpthread

# Make patch utility executable
chmod +x patch_util.sh

# Create initial patch file if it doesn't exist
if [ ! -f "/tmp/floodgate.patch" ]; then
    echo "# Floodgate patch file - functions listed here will be skipped" > /tmp/floodgate.patch
    echo "# Add function addresses one per line (in hex format)" >> /tmp/floodgate.patch
    echo "Created initial patch file at /tmp/floodgate.patch"
fi

# Show function addresses
echo "Listing available functions to patch:"
./patch_util.sh list

# Run the instrumented program
echo ""
echo "Running instrumented program:"
echo "----------------------------"
echo "You can patch functions while it's running by adding their addresses to /tmp/floodgate.patch"
echo "In another terminal, run: ./patch_util.sh add <address>"
echo "To stop patching, run: ./patch_util.sh remove <address>"
echo "To see available functions, run: ./patch_util.sh list"
echo ""
echo "Press Ctrl+C to exit the program"
echo ""

LD_LIBRARY_PATH=../build/lib ./example_program

