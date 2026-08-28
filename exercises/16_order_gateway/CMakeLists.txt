add_library(order_gateway src/order_gateway.cpp)
target_include_directories(order_gateway PUBLIC include)
target_compile_features(order_gateway PUBLIC cxx_std_20)

add_executable(order_gateway_test test/test_order_gateway.cpp)
target_link_libraries(order_gateway_test PRIVATE order_gateway gtest_main)
add_test(NAME order_gateway_test COMMAND order_gateway_test)
set_tests_properties(order_gateway_test PROPERTIES LABELS "exercise16;tsan;stress" TIMEOUT 300)