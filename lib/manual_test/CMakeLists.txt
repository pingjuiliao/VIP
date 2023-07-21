
SET(CMAKE_C_FLAGS "-Wall -m64 -fPIC")

add_executable(
  ovfl_c_fptr.exe ovfl_c_fptr
)

target_include_directories(ovfl_c_fptr.exe
  PUBLIC
  ${PROJECT_SOURCE_DIR}/lib)

target_link_libraries(ovfl_c_fptr.exe VIP)
