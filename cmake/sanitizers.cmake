# Sanitizer toggles. Both default OFF; enabling one flags the build globally
# (including the fetched GoogleTest). -- Configure with:
#   cmake -B build -DENABLE_TSAN=ON    (ThreadSanitizer)
#   cmake -B build -DENABLE_ASAN=ON    (AddressSanitizer)
# TSan and ASan are mutually exclusive.

option(ENABLE_TSAN "Build with ThreadSanitizer" OFF)
option(ENABLE_ASAN "Build with AddressSanitizer" OFF)

if(ENABLE_TSAN AND ENABLE_ASAN)
  message(FATAL_ERROR "ENABLE_TSAN and ENABLE_ASAN are mutually exclusive. Pick one.")
endif()

if(ENABLE_TSAN)
  message(STATUS "Sanitizers: enabling ThreadSanitizer for all targets")
  add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g -O1)
  add_link_options(-fsanitize=thread)
endif()

if(ENABLE_ASAN)
  message(STATUS "Sanitizers: enabling AddressSanitizer for all targets")
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g -O1)
  add_link_options(-fsanitize=address)
endif()