# Floodgate: Dynamic Function Patching Library

Floodgate is a runtime patching system for C/C++ applications that allows you to selectively disable functions in a running program without restarting it. When a function is patched, it will immediately return a default value instead of executing.

## Features

- **Dynamic Patching**: Patch functions at runtime without restarting the application
- **Hot Reloading**: Changes to the patch configuration are detected and applied immediately
- **Return Value Support**: Patched functions return appropriate default values based on their return type
- **Low Overhead**: Minimal performance impact when functions are not patched
- **Simple Configuration**: Just add function addresses to a text file

## How It Works

1. LLVM Pass: The `BacktracePass` instruments each function with a runtime check
2. Runtime Library: The `backtrace_runtime` library implements the patching logic
3. Control File: A simple text file containing function addresses controls what gets patched

## Building

```bash
mkdir -p build && cd build
cmake ..
make
```

## Usage

### Instrumenting Your Program

1. Compile your C/C++ code to LLVM IR:
```bash
clang -emit-llvm -S -c source.c -o source.ll
```

2. Run the instrumentation pass:
```bash
opt -load-pass-plugin=./build/lib/BacktracePass.so -passes=backtrace -S source.ll -o source.instrumented.ll
```

3. Compile to object file:
```bash
llc -filetype=obj source.instrumented.ll -o source.o
```

4. Link with the runtime library:
```bash
clang source.o -o program -L./build/lib -lbacktrace_runtime -lunwind -lpthread
```

5. Run your instrumented program:
```bash
LD_LIBRARY_PATH=./build/lib ./program
```

### Patching Functions

Create a file at `/tmp/floodgate.patch` (default location) with the addresses of functions you want to patch, one per line:

```
# Comment line
0x1234abcd  # function address in hex
0x5678efab  # another function address
```

The program will monitor this file for changes and immediately update its behavior when you add or remove addresses.

### Example

An example program is included to demonstrate the patching system:

```bash
cd example
./run_example.sh
```

In another terminal window, use the patch utility to add/remove functions while the program is running:

```bash
# List available functions
./patch_util.sh list

# Patch a function
./patch_util.sh add 0x12345678

# Remove a patch
./patch_util.sh remove 0x12345678

# Clear all patches
./patch_util.sh clear
```

## How the Patching System Works

1. **Initialization**:
   - At startup, the system creates a bitmap to track which functions should be executed
   - It sets up an inotify watch on the patch file
   - It registers all instrumented functions and their default return values

2. **Runtime Checks**:
   - Each function begins with a check to see if it should be executed
   - If the function is patched, it returns immediately with a default value
   - Otherwise, it continues normal execution

3. **Dynamic Updates**:
   - A background thread monitors the patch file for changes
   - When changes are detected, the bitmap is updated
   - Subsequent function calls will use the new configuration

## Limitations

- Functions must be instrumented at compile time
- The patched program must have access to the patch file
- Inlined functions won't be patchable individually
- The default return value is always a zeroed/null value of the appropriate type

## Dependencies

- LLVM (for the instrumentation pass)
- libunwind (for runtime support)
- pthread (for threading support)

## License

This project is released under the MIT License.