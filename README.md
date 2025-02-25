# Floodgate - Function Entry Backtrace Instrumentation

This project provides an LLVM pass and runtime library to instrument C/C++ code with function entry backtraces.
When a function is entered, it prints the function name and a stack backtrace using libunwind.

## Build Requirements

- LLVM development libraries (11.0.0 or newer)
- libunwind development libraries
- CMake 3.10 or newer

## Building

```bash
mkdir build
cd build
cmake ..
make
```

This will create:
- `lib/BacktracePass.so` - The LLVM pass
- `lib/libbacktrace_runtime.so` - The runtime library

## Usage

### Compile with instrumentation

```bash
# Compile your C/C++ code to LLVM IR
clang -emit-llvm -S -c your_code.c -o your_code.ll

# Run the pass
opt -load ./build/lib/BacktracePass.so -backtrace -S your_code.ll -o your_code.instrumented.ll

# Compile to object file
llc -filetype=obj your_code.instrumented.ll -o your_code.o

# Link with the runtime
clang your_code.o -o your_program -L./build/lib -lbacktrace_runtime -lunwind
```

### Run your instrumented program

```bash
LD_LIBRARY_PATH=./build/lib ./your_program
```

This will print a backtrace each time a function is entered.

## Example Output

```
Entering function: main
Backtrace:
#0: main+0x5 [ip=0x55ea9b4b8ba5] [sp=0x7ffe30bdf830]
#1: __libc_start_main+0xf2 [ip=0x7f17eef36152] [sp=0x7ffe30bdf850]
#2: _start+0x2e [ip=0x55ea9b4b8aae] [sp=0x7ffe30bdf930]

Entering function: foo
Backtrace:
#0: foo+0x5 [ip=0x55ea9b4b8c15] [sp=0x7ffe30bdf800]
#1: main+0x2a [ip=0x55ea9b4b8bca] [sp=0x7ffe30bdf830]
#2: __libc_start_main+0xf2 [ip=0x7f17eef36152] [sp=0x7ffe30bdf850]
#3: _start+0x2e [ip=0x55ea9b4b8aae] [sp=0x7ffe30bdf930]
```
