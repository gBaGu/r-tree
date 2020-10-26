find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_executable(test
    main.cpp
    tree_operations_tests.cpp
    linear_split_tests.cpp
    quadratic_split_tests.cpp
    exponential_split_tests.cpp
)

target_include_directories(test PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(test
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

enable_testing()
add_test(NAME TreeTest COMMAND $<TARGET_FILE:test>)
