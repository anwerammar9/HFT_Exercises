add_library(semaphore src/semaphore.cpp)
target_include_directories(semaphore PUBLIC include)
target_compile_features(semaphore PUBLIC cxx_std_20)

add_executable(semaphore_test test/test_semaphore.cpp)
target_link_libraries(semaphore_test PRIVATE semaphore gtest_main)
add_test(NAME semaphore_test COMMAND semaphore_test)
set_tests_properties(semaphore_test PROPERTIES LABELS "exercise27;tsan;stress" TIMEOUT 300)