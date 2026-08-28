add_library(thread_pool src/thread_pool.cpp)
target_include_directories(thread_pool PUBLIC include)
target_compile_features(thread_pool PUBLIC cxx_std_20)

add_executable(thread_pool_test test/test_thread_pool.cpp)
target_link_libraries(thread_pool_test PRIVATE thread_pool gtest_main)
add_test(NAME thread_pool_test COMMAND thread_pool_test)
set_tests_properties(thread_pool_test PROPERTIES LABELS "exercise14" TIMEOUT 300)