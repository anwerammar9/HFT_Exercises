add_library(thread_pool src/thread_pool.cpp)
target_include_directories(thread_pool PUBLIC include)
target_compile_features(thread_pool PUBLIC cxx_std_20)

add_executable(thread_pool_test test/test_thread_pool.cpp)
target_link_libraries(thread_pool_test PRIVATE thread_pool gtest_main)
add_test(NAME thread_pool_test COMMAND thread_pool_test)
set_tests_properties(thread_pool_test PROPERTIES LABELS "exercise14" TIMEOUT 300)

# Optional Google Benchmark micro-bench (submit throughput vs worker count),
# only built when requested. Separate from the test target on purpose.
option(BUILD_THREAD_POOL_BENCH "Build the optional thread pool benchmark" OFF)
if(BUILD_THREAD_POOL_BENCH)
  include(FetchContent)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
  )
  FetchContent_MakeAvailable(benchmark)

  add_executable(thread_pool_bench bench/thread_pool_bench.cpp)
  target_link_libraries(thread_pool_bench PRIVATE thread_pool benchmark::benchmark_main)
  target_compile_features(thread_pool_bench PRIVATE cxx_std_20)
endif()