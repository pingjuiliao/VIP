SET(CMAKE_C_COMPILER clang)
SET(CMAKE_CXX_COMPILER clang++)

SET(CMAKE_C_FLAGS "-Wall -shared -m64 -fPIC -O3")

add_library(VIP SHARED
  vip.cpp
  vip.h
)

add_subdirectory(manual_test)
