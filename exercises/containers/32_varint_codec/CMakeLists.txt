add_library(varint_codec src/varint_codec.cpp)
target_include_directories(varint_codec PUBLIC include)
target_compile_features(varint_codec PUBLIC cxx_std_20)

add_executable(varint_codec_test test/test_varint_codec.cpp)
target_link_libraries(varint_codec_test PRIVATE varint_codec gtest_main)
add_test(NAME varint_codec_test COMMAND varint_codec_test)
set_tests_properties(varint_codec_test PROPERTIES LABELS "exercise32" TIMEOUT 300)