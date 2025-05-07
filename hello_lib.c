#include <stdio.h>
#include <stdlib.h>
void run_node_hello();

//Function that called from the main function
void run_node_hello() {
    int result = system("node hello.js");
    if (result != 0) {
        fprintf(stderr, "failed Node.js\n");
    }
}



int main() {
    printf("run from c dynamic library\n");
    run_node_hello();
    return 0;
}
