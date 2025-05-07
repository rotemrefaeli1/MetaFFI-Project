cmake_minimum_required(VERSION 3.30)
project(MetaFFI_Project VERSION 0.0.1)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

add_library(MetaFFI_Project SHARED library.c
        runtime/plugin_interface.cpp)

add_subdirectory(plugin-sdk-main)
