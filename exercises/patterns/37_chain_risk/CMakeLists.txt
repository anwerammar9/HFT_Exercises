add_library(chain_risk src/chain_risk.cpp)
target_include_directories(chain_risk PUBLIC include)
target_compile_features(chain_risk PUBLIC cxx_std_20)

add_executable(chain_risk_test test/test_chain_risk.cpp)
target_link_libraries(chain_risk_test PRIVATE chain_risk gtest_main)
add_test(NAME chain_risk_test COMMAND chain_risk_test)
set_tests_properties(chain_risk_test PROPERTIES LABELS "exercise37" TIMEOUT 300)