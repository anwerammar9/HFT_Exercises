add_library(linked_list src/linked_list.cpp)
target_include_directories(linked_list PUBLIC include)
target_compile_features(linked_list PUBLIC cxx_std_20)

add_executable(linked_list_test test/test_linked_list.cpp)
target_link_libraries(linked_list_test PRIVATE linked_list gtest_main)
add_test(NAME linked_list_test COMMAND linked_list_test)
set_tests_properties(linked_list_test PROPERTIES LABELS "exercise38" TIMEOUT 300)