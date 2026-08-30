add_library(spin_barrier src/spin_barrier.cpp)
target_include_directories(spin_barrier PUBLIC include)
target_compile_features(spin_barrier PUBLIC cxx_std_20)

add_executable(spin_barrier_test test/test_spin_barrier.cpp)
target_link_libraries(spin_barrier_test PRIVATE spin_barrier gtest_main)
add_test(NAME spin_barrier_test COMMAND spin_barrier_test)
set_tests_properties(spin_barrier_test PROPERTIES LABELS "exercise28;tsan;stress" TIMEOUT 300)