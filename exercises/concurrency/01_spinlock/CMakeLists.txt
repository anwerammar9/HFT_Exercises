add_library(spinlock src/spinlock.cpp)
target_include_directories(spinlock PUBLIC include)
target_compile_features(spinlock PUBLIC cxx_std_20)

add_executable(spinlock_test test/test_spinlock.cpp)
target_link_libraries(spinlock_test PRIVATE spinlock gtest_main)
add_test(NAME spinlock_test COMMAND spinlock_test)
set_tests_properties(spinlock_test PROPERTIES LABELS "exercise11" TIMEOUT 300)

# Optional Google Benchmark micro-bench (naive TAS vs TTAS vs TTAS+backoff),
# only built when requested. Separate from the test target on purpose.
option(BUILD_SPINLOCK_BENCH "Build the optional spinlock benchmark" OFF)
if(BUILD_SPINLOCK_BENCH)
  include(FetchContent)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.3
  )
  FetchContent_MakeAvailable(benchmark)

  add_executable(spinlock_bench bench/spinlock_bench.cpp)
  target_link_libraries(spinlock_bench PRIVATE spinlock benchmark::benchmark_main)
  target_compile_features(spinlock_bench PRIVATE cxx_std_20)
endif()