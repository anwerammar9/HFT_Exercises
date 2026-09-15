add_library(arena_gc src/arena_gc.cpp)
target_include_directories(arena_gc PUBLIC include)
target_compile_features(arena_gc PUBLIC cxx_std_20)

add_executable(arena_gc_test test/test_arena_gc.cpp)
target_link_libraries(arena_gc_test PRIVATE arena_gc gtest_main)
add_test(NAME arena_gc_test COMMAND arena_gc_test)
set_tests_properties(arena_gc_test PROPERTIES LABELS "exercise49" TIMEOUT 300)
