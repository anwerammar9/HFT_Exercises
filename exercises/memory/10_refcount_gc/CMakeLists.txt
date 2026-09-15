add_library(refcount_gc src/refcount_gc.cpp)
target_include_directories(refcount_gc PUBLIC include)
target_compile_features(refcount_gc PUBLIC cxx_std_20)

add_executable(refcount_gc_test test/test_refcount_gc.cpp)
target_link_libraries(refcount_gc_test PRIVATE refcount_gc gtest_main)
add_test(NAME refcount_gc_test COMMAND refcount_gc_test)
set_tests_properties(refcount_gc_test PROPERTIES LABELS "exercise51" TIMEOUT 300)
