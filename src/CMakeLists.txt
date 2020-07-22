add_executable(r-tree main.cpp)

target_compile_options(r-tree
    PRIVATE
    "-Wall"
)

target_include_directories(r-tree PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)