cmake_minimum_required(VERSION 3.20)
project(MetaFFI_Project VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Include directories for Node.js headers
set(test_nodejs_src ${CMAKE_CURRENT_LIST_DIR}/tests/main.cpp)

# Executable to test nodejs_api
set(test_nodejs_api_src ${CMAKE_CURRENT_LIST_DIR}/tests/test_nodejs_api.cpp)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")



# Executable to test Node.js embedding
add_executable(test_nodejs ${test_nodejs_src})

add_executable(test_nodejs_api ${test_nodejs_api_src})



target_include_directories(
        test_nodejs PRIVATE
        "C:/node-v20.10.0/src"
        "C:/node-v20.10.0/deps/v8/include"
        "C:/node-v20.10.0/deps/uv/include"
)

link_directories(
        "C:/node-v20.10.0/Release"
        "C:/node-v20.10.0/Release/lib"
)


# Link with the correct library name:
target_link_libraries(test_nodejs PRIVATE
       # "C:/node-v20.10.0/Release/node.lib"
        "C:/node-v20.10.0/out/Release/lib/v8_libplatform.lib"
        "C:/node-v20.10.0/out/Release/libnode.lib"
)

#link with the libraries
target_include_directories(
        test_nodejs_api PRIVATE
        "C:/node-v20.10.0/src"
        "C:/node-v20.10.0/deps/v8/include"
        "C:/node-v20.10.0/deps/uv/include"
)

target_link_libraries(test_nodejs_api PRIVATE
        "C:/node-v20.10.0/out/Release/lib/v8_libplatform.lib"
        "C:/node-v20.10.0/out/Release/libnode.lib"
)