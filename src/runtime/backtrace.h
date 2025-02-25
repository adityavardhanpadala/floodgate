#ifndef BACKTRACE_H
#define BACKTRACE_H

/**
 * Print a backtrace to stderr when entering a function
 * @param func_name The name of the function being entered
 */
void print_backtrace(const char *func_name);

/**
 * Enable or disable backtracing at runtime
 * @param enable 1 to enable, 0 to disable
 */
void backtrace_enable(int enable);

/**
 * Print a backtrace if tracing is enabled
 * @param func_name The name of the function being entered
 */
void print_backtrace_if_enabled(const char *func_name);

#endif /* BACKTRACE_H */