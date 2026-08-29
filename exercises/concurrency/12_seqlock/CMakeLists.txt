add_library(seqlock src/seqlock.cpp src/rw_spinlock.cpp)
target_include_directories(seqlock PUBLIC include)
target_compile_features(seqlock PUBLIC cxx_std_20)
target_link_libraries(seqlock PRIVATE atomic)

add_executable(seqlock_test test/test_seqlock.cpp)
target_link_libraries(seqlock_test PRIVATE seqlock gtest_main atomic)
add_test(NAME seqlock_test COMMAND seqlock_test)
set_tests_properties(seqlock_test PROPERTIES LABELS "exercise12" TIMEOUT 300)