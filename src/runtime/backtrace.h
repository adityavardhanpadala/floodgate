#ifndef BACKTRACE_H
#define BACKTRACE_H

#include <stdint.h>

/**
 * Check if a function should be executed
 * @param func_addr The address of the function
 * @return 1 if function should execute, 0 if it should be skipped
 */
int should_execute_function(uintptr_t func_addr);

/**
 * Initialize the patching system
 * @param patch_file Path to the patch control file
 */
void initialize_patch_system(const char *patch_file);

/**
 * Register a function with the patching system (called at program start)
 * @param func_addr The address of the function
 * @param func_name The name of the function
 * @param return_type_size Size of return type (0 for void)
 * @param default_return_value Pointer to default return value (NULL for void)
 */
void register_patchable_function(uintptr_t func_addr, const char *func_name, 
                                uint8_t return_type_size, void *default_return_value);

/**
 * Get default return value for a skipped function
 * @param func_addr The address of the function
 * @return Pointer to the default return value, or NULL for void functions
 */
void* get_default_return_value(uintptr_t func_addr);

#endif /* BACKTRACE_H */