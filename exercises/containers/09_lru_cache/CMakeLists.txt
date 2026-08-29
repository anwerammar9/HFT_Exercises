add_library(lru_cache src/lru_cache.cpp)
target_include_directories(lru_cache PUBLIC include)
target_compile_features(lru_cache PUBLIC cxx_std_20)

add_executable(lru_cache_test test/test_lru_cache.cpp)
target_link_libraries(lru_cache_test PRIVATE lru_cache gtest_main)
add_test(NAME lru_cache_test COMMAND lru_cache_test)
set_tests_properties(lru_cache_test PROPERTIES LABELS "exercise09" TIMEOUT 300)