#include <iostream>
#include <memory> // for std::shared_ptr and std::make_shared
#include <cstring> // for strdup

#include <cstdlib>

void load_runtime(char** err)
{
    try
    {
        *err = nullptr;
        std::cout << "Node.js runtime initialized" << std::endl;
        // other risky logic here
    }
    catch (const std::exception& e)
    {
        *err = strdup(e.what()); // don't forget <cstring>
    }
}

void free_runtime(char** err)
{
    try
    {
        *err = nullptr;
        std::cout << "Node.js runtime finalized" << std::endl;
        // If you had allocated any Node.js resources (like V8 isolate or event loop),
        // you'd release them here.
    }
    catch (const std::exception& e)
    {
        *err = strdup(e.what());
    }
}



int main()
{
    char* err = nullptr;

    std::cout << "Calling load_runtime..." << std::endl;
    load_runtime(&err);
    if (err)
    {
        std::cerr << "Error in load_runtime: " << err << std::endl;
        free(err);
        return 1;
    }

    std::cout << "Calling free_runtime..." << std::endl;
    free_runtime(&err);
    if (err)
    {
        std::cerr << "Error in free_runtime: " << err << std::endl;
        free(err);
        return 1;
    }

    std::cout << "Runtime test completed successfully." << std::endl;
    std::cout.flush();
    return 0;
}
