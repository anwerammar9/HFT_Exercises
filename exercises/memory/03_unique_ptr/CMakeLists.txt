add_library(unique_ptr src/unique_ptr.cpp)
target_include_directories(unique_ptr PUBLIC include)
target_compile_features(unique_ptr PUBLIC cxx_std_20)

add_executable(unique_ptr_test test/test_unique_ptr.cpp)
target_link_libraries(unique_ptr_test PRIVATE unique_ptr gtest_main)
add_test(NAME unique_ptr_test COMMAND unique_ptr_test)
set_tests_properties(unique_ptr_test PROPERTIES LABELS "exercise44" TIMEOUT 300)
