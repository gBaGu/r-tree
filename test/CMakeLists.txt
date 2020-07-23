find_package(Boost COMPONENTS unit_test_framework REQUIRED)

add_executable(test test.cpp)

target_include_directories(test PUBLIC
    "${PROJECT_SOURCE_DIR}/include"
)
target_link_libraries(test
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)

add_test(TreeTest test)
