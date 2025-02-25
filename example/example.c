#include <stdio.h>

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

int main() {
    printf("Starting example program\n");
    level1();
    printf("Finished example program\n");
    return 0;
}