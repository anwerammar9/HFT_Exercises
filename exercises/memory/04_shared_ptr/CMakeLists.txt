add_library(shared_ptr src/shared_ptr.cpp)
target_include_directories(shared_ptr PUBLIC include)
target_compile_features(shared_ptr PUBLIC cxx_std_20)

add_executable(shared_ptr_test test/test_shared_ptr.cpp)
target_link_libraries(shared_ptr_test PRIVATE shared_ptr gtest_main)
add_test(NAME shared_ptr_test COMMAND shared_ptr_test)
set_tests_properties(shared_ptr_test PROPERTIES LABELS "exercise45" TIMEOUT 300)
