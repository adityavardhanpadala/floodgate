#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Global context initialization flag
static int g_backtrace_initialized = 0;
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;

// Initialize once
static void ensure_initialized() {
    if (!g_backtrace_initialized) {
        pthread_mutex_lock(&init_mutex);
        if (!g_backtrace_initialized) {
            // Any one-time libunwind initialization can go here
            g_backtrace_initialized = 1;
        }
        pthread_mutex_unlock(&init_mutex);
    }
}

void print_backtrace(const char *func_name) {
    unw_cursor_t cursor;
    unw_context_t context;
    unw_word_t ip, sp;
    unw_word_t offset;
    char symbol[256];
    
    // Ensure initialization is done only once
    ensure_initialized();
    
    // Print the function name that's being entered
    fprintf(stderr, "Entering function: %s\n", func_name);
    fprintf(stderr, "Backtrace:\n");

    // Get current context
    unw_getcontext(&context);
    unw_init_local(&cursor, &context);
    
    // Walk the stack
    int frame = 0;
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        
        // Get function name
        if (unw_get_proc_name(&cursor, symbol, sizeof(symbol), &offset) == 0) {
            fprintf(stderr, "#%d: %s+0x%lx [ip=0x%lx] [sp=0x%lx]\n", 
                    frame, symbol, (long)offset, (long)ip, (long)sp);
        } else {
            fprintf(stderr, "#%d: <unknown> [ip=0x%lx] [sp=0x%lx]\n", 
                    frame, (long)ip, (long)sp);
        }
        
        frame++;
    }
    
    fprintf(stderr, "\n");
}

// Function to enable/disable backtrace at runtime
static int backtrace_enabled = 1;

void backtrace_enable(int enable) {
    backtrace_enabled = enable;
}

// Modified version that checks if backtrace is enabled
void print_backtrace_if_enabled(const char *func_name) {
    if (backtrace_enabled) {
        print_backtrace(func_name);
    }
}