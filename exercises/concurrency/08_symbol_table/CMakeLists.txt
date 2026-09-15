add_library(symbol_table src/symbol_table.cpp)
target_include_directories(symbol_table PUBLIC include)
target_compile_features(symbol_table PUBLIC cxx_std_20)

add_executable(symbol_table_test test/test_symbol_table.cpp)
target_link_libraries(symbol_table_test PRIVATE symbol_table gtest_main)
add_test(NAME symbol_table_test COMMAND symbol_table_test)
set_tests_properties(symbol_table_test PROPERTIES LABELS "exercise25;tsan;stress" TIMEOUT 300)