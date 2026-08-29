add_library(mcs_lock src/mcs_lock.cpp)
target_include_directories(mcs_lock PUBLIC include)
target_compile_features(mcs_lock PUBLIC cxx_std_20)

add_executable(mcs_lock_test test/test_mcs_lock.cpp)
target_link_libraries(mcs_lock_test PRIVATE mcs_lock gtest_main)
add_test(NAME mcs_lock_test COMMAND mcs_lock_test)
set_tests_properties(mcs_lock_test PROPERTIES LABELS "exercise29;tsan;stress" TIMEOUT 300)