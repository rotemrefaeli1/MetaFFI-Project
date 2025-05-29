#include "library.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

void hello(void) {
    printf("Hello, World!\n");
}

void uppercase(const char* input, char* output) {
    int i = 0;
    while (input[i] != '\0') {
        output[i] = toupper((unsigned char)input[i]);
        i++;
    }
    output[i] = '\0';  // Null-terminate the output
}

int add(int a, int b, int c) {
    return a + b + c;
}

int integer_div(int a, int b) {
    if (b == 0) {
        return 0;  // Handle division by 0
    }
    return a / b;
}
