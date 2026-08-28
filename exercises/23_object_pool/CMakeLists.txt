add_library(object_pool src/object_pool.cpp)
target_include_directories(object_pool PUBLIC include)
target_compile_features(object_pool PUBLIC cxx_std_20)

add_executable(object_pool_test test/test_object_pool.cpp)
target_link_libraries(object_pool_test PRIVATE object_pool gtest_main)
add_test(NAME object_pool_test COMMAND object_pool_test)
set_tests_properties(object_pool_test PROPERTIES LABELS "exercise23;tsan;stress" TIMEOUT 300)