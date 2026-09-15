cmake_minimum_required(VERSION 3.16)
project(concurrency_playground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(cmake/warnings.cmake)
include(cmake/sanitizers.cmake)

enable_testing()
find_package(Threads REQUIRED)

# GoogleTest is fetched, not vendored (see README). Requires network on first configure.
include(FetchContent)
set(GTEST_TAG v1.15.2)
set(gtest_build_tests OFF CACHE BOOL "" FORCE)
set(gmock_build_tests OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG ${GTEST_TAG}
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(exercises)