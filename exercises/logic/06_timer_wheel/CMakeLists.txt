add_library(timer_wheel src/timer_wheel.cpp)
target_include_directories(timer_wheel PUBLIC include)
target_compile_features(timer_wheel PUBLIC cxx_std_20)

add_executable(timer_wheel_test test/test_timer_wheel.cpp)
target_link_libraries(timer_wheel_test PRIVATE timer_wheel gtest_main)
add_test(NAME timer_wheel_test COMMAND timer_wheel_test)
set_tests_properties(timer_wheel_test PROPERTIES LABELS "exercise10" TIMEOUT 300)