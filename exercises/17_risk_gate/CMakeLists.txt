add_library(risk_gate src/risk_gate.cpp)
target_include_directories(risk_gate PUBLIC include)
target_compile_features(risk_gate PUBLIC cxx_std_20)

add_executable(risk_gate_test test/test_risk_gate.cpp)
target_link_libraries(risk_gate_test PRIVATE risk_gate gtest_main)
add_test(NAME risk_gate_test COMMAND risk_gate_test)
set_tests_properties(risk_gate_test PROPERTIES LABELS "exercise17;tsan;stress" TIMEOUT 300)