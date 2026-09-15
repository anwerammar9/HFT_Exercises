add_library(borrow_guards src/borrow_guards.cpp)
target_include_directories(borrow_guards PUBLIC include)
target_compile_features(borrow_guards PUBLIC cxx_std_20)

add_executable(borrow_guards_test test/test_borrow_guards.cpp)
target_link_libraries(borrow_guards_test PRIVATE borrow_guards gtest_main)
add_test(NAME borrow_guards_test COMMAND borrow_guards_test)
set_tests_properties(borrow_guards_test PROPERTIES LABELS "exercise46" TIMEOUT 300)
