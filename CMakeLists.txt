cmake_minimum_required(VERSION 3.20)
project(MetaFFI_Project VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

# Include directories for Node.js headers
include_directories(
        ${CMAKE_SOURCE_DIR}/include
        "C:/Users/sagib/OneDrive/Documents/node-v20.10.0/src"
        "C:/Users/sagib/OneDrive/Documents/node-v20.10.0/deps/v8/include"

)

# Link directories where libnode.lib is located
link_directories(
        "C:/Users/sagib/OneDrive/Documents/node-v20.10.0/out/Release/lib"
)

# Executable to test Node.js embedding
add_executable(run_nodejs tests/main.cpp)

# Link with the correct library name:
target_link_libraries(run_nodejs PRIVATE libnode)
# or: target_link_libraries(run_nodejs PRIVATE node) ← לפי שם הקובץ
