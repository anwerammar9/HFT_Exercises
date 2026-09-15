add_library(state_borrow src/state_borrow.cpp)
target_include_directories(state_borrow PUBLIC include)
target_compile_features(state_borrow PUBLIC cxx_std_20)

add_executable(state_borrow_test test/test_state_borrow.cpp)
target_link_libraries(state_borrow_test PRIVATE state_borrow gtest_main)
add_test(NAME state_borrow_test COMMAND state_borrow_test)
set_tests_properties(state_borrow_test PROPERTIES LABELS "exercise48" TIMEOUT 300)
