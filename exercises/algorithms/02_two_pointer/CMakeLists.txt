add_library(two_pointer src/two_pointer.cpp)
target_include_directories(two_pointer PUBLIC include)
target_compile_features(two_pointer PUBLIC cxx_std_20)

add_executable(two_pointer_test test/test_two_pointer.cpp)
target_link_libraries(two_pointer_test PRIVATE two_pointer gtest_main)
add_test(NAME two_pointer_test COMMAND two_pointer_test)
set_tests_properties(two_pointer_test PROPERTIES LABELS "exercise39" TIMEOUT 300)