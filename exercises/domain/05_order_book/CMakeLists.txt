add_library(order_book src/order_book.cpp)
target_include_directories(order_book PUBLIC include)
target_compile_features(order_book PUBLIC cxx_std_20)

add_executable(order_book_test test/test_order_book.cpp)
target_link_libraries(order_book_test PRIVATE order_book gtest_main)
add_test(NAME order_book_test COMMAND order_book_test)
set_tests_properties(order_book_test PROPERTIES LABELS "exercise19" TIMEOUT 300)