add_library(mark_sweep src/mark_sweep.cpp)
target_include_directories(mark_sweep PUBLIC include)
target_compile_features(mark_sweep PUBLIC cxx_std_20)

add_executable(mark_sweep_test test/test_mark_sweep.cpp)
target_link_libraries(mark_sweep_test PRIVATE mark_sweep gtest_main)
add_test(NAME mark_sweep_test COMMAND mark_sweep_test)
set_tests_properties(mark_sweep_test PROPERTIES LABELS "exercise50" TIMEOUT 300)
