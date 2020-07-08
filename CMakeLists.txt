cmake_minimum_required(VERSION 3.10)
project(r-tree VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_subdirectory(src)

target_include_directories(r-tree PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)