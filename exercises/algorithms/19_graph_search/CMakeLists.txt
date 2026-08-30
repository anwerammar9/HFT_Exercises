add_library(graph_search src/graph_search.cpp)
target_include_directories(graph_search PUBLIC include)
target_compile_features(graph_search PUBLIC cxx_std_20)

add_executable(graph_search_test test/test_graph_search.cpp)
target_link_libraries(graph_search_test PRIVATE graph_search gtest_main)
add_test(NAME graph_search_test COMMAND graph_search_test)
set_tests_properties(graph_search_test PROPERTIES LABELS "exercise42" TIMEOUT 300)