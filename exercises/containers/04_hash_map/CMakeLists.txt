add_library(hash_map src/hash_map.cpp)
target_include_directories(hash_map PUBLIC include)
target_compile_features(hash_map PUBLIC cxx_std_20)

add_executable(hash_map_test test/test_hash_map.cpp)
target_link_libraries(hash_map_test PRIVATE hash_map gtest_main)
add_test(NAME hash_map_test COMMAND hash_map_test)
set_tests_properties(hash_map_test PROPERTIES LABELS "exercise24" TIMEOUT 300)