#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include "backtrace.h"

// Maximum number of functions that can be patched
#define MAX_FUNCTIONS 4096

// Bitmap to track function execution status (1 = execute, 0 = skip)
static uint8_t *function_bitmap = NULL;
static size_t bitmap_size = 0;

// Map of function addresses to their default return values
typedef struct {
    uintptr_t func_addr;
    char *func_name;
    uint8_t return_type_size;
    void *default_return_value;
} function_info_t;

static function_info_t function_registry[MAX_FUNCTIONS];
static size_t registered_functions = 0;

// Path to the patch file
static char patch_file_path[PATH_MAX];

// Monitor thread
static pthread_t monitor_thread;
static int monitor_running = 0;

// Initialization protection
static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t bitmap_mutex = PTHREAD_MUTEX_INITIALIZER;
static int system_initialized = 0;

// Track file modification time to detect changes
static time_t last_mtime = 0;

// Function to update the bitmap based on patch file contents
static void update_bitmap_from_file() {
    FILE *file = fopen(patch_file_path, "r");
    if (!file) {
        // File might not exist, which is normal for initial setup
        if (errno != ENOENT) {
            fprintf(stderr, "Warning: Could not open patch file %s: %s\n", 
                    patch_file_path, strerror(errno));
        }
        return;
    }
    
    // Acquire lock before modifying bitmap
    pthread_mutex_lock(&bitmap_mutex);
    
    // Reset all functions to enabled (1) by default
    memset(function_bitmap, 0xFF, bitmap_size);
    
    // Read addresses from file and mark them as disabled (0)
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        line[strcspn(line, "\n")] = 0;
        
        // Skip empty lines and comments
        if (line[0] == 0 || line[0] == '#')
            continue;
            
        // Try to parse as hex address
        uintptr_t addr = strtoull(line, NULL, 16);
        if (addr == 0)
            continue;
            
        // Find the function in our registry
        for (size_t i = 0; i < registered_functions; i++) {
            if (function_registry[i].func_addr == addr) {
                // Calculate byte and bit position
                size_t byte_index = i / 8;
                uint8_t bit_mask = 1 << (i % 8);
                
                // Clear the bit to disable the function
                function_bitmap[byte_index] &= ~bit_mask;
                fprintf(stderr, "Patched function: %s (0x%lx)\n", 
                        function_registry[i].func_name, (unsigned long)addr);
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&bitmap_mutex);
    fclose(file);
}

// Thread function to monitor the patch file for changes
static void* monitor_patch_file(void *arg) {
    while (monitor_running) {
        // Check file modification time
        struct stat st;
        if (stat(patch_file_path, &st) == 0) {
            if (st.st_mtime != last_mtime) {
                // File has been modified
                fprintf(stderr, "Patch file updated, reloading...\n");
                last_mtime = st.st_mtime;
                update_bitmap_from_file();
            }
        }
        
        // Sleep for a short time before checking again
        usleep(500000); // 500ms
    }
    
    return NULL;
}

void initialize_patch_system(const char *patch_file) {
    pthread_mutex_lock(&init_mutex);
    
    if (system_initialized) {
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    
    // Copy patch file path
    strncpy(patch_file_path, patch_file, PATH_MAX - 1);
    patch_file_path[PATH_MAX - 1] = '\0';
    
    // Allocate bitmap (1 bit per function)
    bitmap_size = (MAX_FUNCTIONS + 7) / 8;  // Ceiling division to bytes
    function_bitmap = (uint8_t*)malloc(bitmap_size);
    if (!function_bitmap) {
        perror("Failed to allocate function bitmap");
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    
    // Initialize bitmap to all 1s (all functions enabled)
    memset(function_bitmap, 0xFF, bitmap_size);
    
    // Check initial file state
    struct stat st;
    if (stat(patch_file_path, &st) == 0) {
        last_mtime = st.st_mtime;
    }
    
    // Load initial patch file if it exists
    update_bitmap_from_file();
    
    // Start monitoring thread
    monitor_running = 1;
    if (pthread_create(&monitor_thread, NULL, monitor_patch_file, NULL) != 0) {
        perror("Failed to create monitoring thread");
        free(function_bitmap);
        function_bitmap = NULL;
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    
    system_initialized = 1;
    pthread_mutex_unlock(&init_mutex);
}

void register_patchable_function(uintptr_t func_addr, const char *func_name, 
                                uint8_t return_type_size, void *default_return_value) {
    // Ensure the system is initialized first
    if (!system_initialized) {
        // Use a default path if not initialized yet
        initialize_patch_system("/tmp/floodgate.patch");
    }
    
    pthread_mutex_lock(&init_mutex);
    
    if (registered_functions >= MAX_FUNCTIONS) {
        fprintf(stderr, "Warning: Maximum number of patchable functions reached\n");
        pthread_mutex_unlock(&init_mutex);
        return;
    }
    
    // Store function information
    function_registry[registered_functions].func_addr = func_addr;
    function_registry[registered_functions].func_name = strdup(func_name);
    function_registry[registered_functions].return_type_size = return_type_size;
    
    // Copy default return value if provided
    if (default_return_value && return_type_size > 0) {
        void *default_value_copy = malloc(return_type_size);
        if (default_value_copy) {
            memcpy(default_value_copy, default_return_value, return_type_size);
            function_registry[registered_functions].default_return_value = default_value_copy;
        } else {
            function_registry[registered_functions].default_return_value = NULL;
        }
    } else {
        function_registry[registered_functions].default_return_value = NULL;
    }
    
    registered_functions++;
    pthread_mutex_unlock(&init_mutex);
}

int should_execute_function(uintptr_t func_addr) {
    if (!system_initialized)
        return 1;  // If not initialized, always execute
    
    // Find the function index
    size_t func_index = SIZE_MAX;
    for (size_t i = 0; i < registered_functions; i++) {
        if (function_registry[i].func_addr == func_addr) {
            func_index = i;
            break;
        }
    }
    
    // If function not found in registry, always execute
    if (func_index == SIZE_MAX)
        return 1;
    
    // Check bitmap (with lock)
    pthread_mutex_lock(&bitmap_mutex);
    size_t byte_index = func_index / 8;
    uint8_t bit_mask = 1 << (func_index % 8);
    int should_execute = (function_bitmap[byte_index] & bit_mask) != 0;
    pthread_mutex_unlock(&bitmap_mutex);
    
    return should_execute;
}

void* get_default_return_value(uintptr_t func_addr) {
    if (!system_initialized)
        return NULL;
    
    // Find the function
    for (size_t i = 0; i < registered_functions; i++) {
        if (function_registry[i].func_addr == func_addr) {
            return function_registry[i].default_return_value;
        }
    }
    
    return NULL;
}