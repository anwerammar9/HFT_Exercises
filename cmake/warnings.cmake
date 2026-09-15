# Optional compile-warning policy. Warnings are enabled for GNU/Clang; -Werror is off by default.

option(ENABLE_WERROR "Treat compiler warnings as errors" OFF)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(ENABLE_WERROR)
    add_compile_options(-Werror)
  endif()
endif()