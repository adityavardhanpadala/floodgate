#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Return examples for different return types
int return_int() {
    printf("Executing return_int, returning 42\n");
    return 42;
}

float return_float() {
    printf("Executing return_float, returning 3.14\n");
    return 3.14f;
}

char* return_string() {
    printf("Executing return_string, returning \"Hello\"\n");
    return "Hello";
}

typedef struct {
    int x;
    float y;
} point_t;

point_t return_struct() {
    printf("Executing return_struct, returning {10, 20.5}\n");
    point_t p = {10, 20.5f};
    return p;
}

void void_function() {
    printf("Executing void_function\n");
}

// Function hierarchy for call stack example
void level3() {
    printf("Inside level3\n");
}

void level2() {
    printf("Inside level2\n");
    level3();
}

void level1() {
    printf("Inside level1\n");
    level2();
}

// Flag to control program exit
volatile int running = 1;

void handle_sigint(int sig) {
    printf("\nReceived SIGINT, exiting...\n");
    running = 0;
}

int main() {
    // Set up signal handler
    signal(SIGINT, handle_sigint);
    
    printf("Starting example program\n");
    printf("This program demonstrates function patching.\n");
    printf("To patch functions, add their addresses to /tmp/floodgate.patch\n");
    printf("Press Ctrl+C to exit\n\n");
    
    // Main loop for continuous testing
    int counter = 0;
    while (running) {
        printf("\n--- Iteration %d ---\n", counter++);
        
        // Call all test functions
        int int_result = return_int();
        printf("  return_int result: %d\n", int_result);
        
        float float_result = return_float();
        printf("  return_float result: %f\n", float_result);
        
        char* str_result = return_string();
        printf("  return_string result: %s\n", str_result);
        
        point_t struct_result = return_struct();
        printf("  return_struct result: {%d, %f}\n", struct_result.x, struct_result.y);
        
        void_function();
        
        // Call the hierarchy once per 5 iterations
        if (counter % 5 == 0) {
            printf("\nCalling function hierarchy:\n");
            level1();
        }
        
        // Sleep for a moment
        sleep(2);
    }
    
    printf("Finished example program\n");
    return 0;
}