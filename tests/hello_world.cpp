// hello_world.cpp
#include <iostream>
#include <cctype>   // for toupper
#include <cstring>  // for strlen

extern "C" {

    // Prints a message from C++
    // void hello_world() {
    //     std::cout << "Hello from C++!" << std::endl;
    //
    // }
    void hello_world() {
        printf("Hello from C++!\n");
        fflush(stdout);
    }

    // Function that receives a string and returns it in uppercase
    void uppercase(const char* input, char* output) {
        int i = 0;
        while (input[i] != '\0') {
            output[i] = toupper(static_cast<unsigned char>(input[i]));
            i++;
        }
        output[i] = '\0';  // End of string
    }

    // Addition function for three integers
    int add(int a, int b, int c) {
        return a + b + c;
    }

    // Integer division function with division-by-zero handling
    int integer_div(int a, int b) {
        if (b == 0) {
            return 0;  // Don't throw an error – just return 0
        }
        return a / b;
    }

}
