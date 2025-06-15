#include "../runtime/nodejs_api.cpp"
#include <iostream>

int main() {
    initialize_environment();
    load_runtime("simple_function.js");

    call_js_function("sayMyName", 0, nullptr);

    free_runtime();
    return 0;
}
