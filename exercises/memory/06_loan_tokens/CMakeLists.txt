add_library(loan_tokens src/loan_tokens.cpp)
target_include_directories(loan_tokens PUBLIC include)
target_compile_features(loan_tokens PUBLIC cxx_std_20)

add_executable(loan_tokens_test test/test_loan_tokens.cpp)
target_link_libraries(loan_tokens_test PRIVATE loan_tokens gtest_main)
add_test(NAME loan_tokens_test COMMAND loan_tokens_test)
set_tests_properties(loan_tokens_test PROPERTIES LABELS "exercise47" TIMEOUT 300)
