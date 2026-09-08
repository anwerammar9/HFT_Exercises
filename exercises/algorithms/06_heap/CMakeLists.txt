add_library(heap src/heap.cpp)
target_include_directories(heap PUBLIC include)
target_compile_features(heap PUBLIC cxx_std_20)

add_executable(heap_test test/test_heap.cpp)
target_link_libraries(heap_test PRIVATE heap gtest_main)
add_test(NAME heap_test COMMAND heap_test)
set_tests_properties(heap_test PROPERTIES LABELS "exercise43" TIMEOUT 300)