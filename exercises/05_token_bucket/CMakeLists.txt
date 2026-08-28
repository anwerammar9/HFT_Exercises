add_library(token_bucket src/token_bucket.cpp)
target_include_directories(token_bucket PUBLIC include)
target_compile_features(token_bucket PUBLIC cxx_std_20)

add_executable(token_bucket_test test/test_token_bucket.cpp)
target_link_libraries(token_bucket_test PRIVATE token_bucket gtest_main)
add_test(NAME token_bucket_test COMMAND token_bucket_test)
set_tests_properties(token_bucket_test PROPERTIES LABELS "exercise05;tsan;stress" TIMEOUT 300)