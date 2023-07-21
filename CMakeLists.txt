cmake_minimum_required(VERSION 3.11)

project(VIP VERSION 1.0.0)

# paths
SET(PROJ_DIR ${PROJECT_SOURCE_DIR})
SET(LLVM_DIR ${PROJ_DIR}/llvm-project)
execute_process(COMMAND nproc OUTPUT_VARIABLE NJOB)

add_custom_target(llvm-rebuild
  COMMAND cd ${LLVM_DIR}/ && rm -rf build
  COMMAND mkdir ${LLVM_DIR}/build
  COMMAND cd ${LLVM_DIR}/build && cmake
          -DCMAKE_BUILD_TYPE=Release
          -DLLVM_ENABLE_PROJECTS="clang;compiler-rt"
          -DBUILD_SHARED_LIBS=On
          -DLLVM_BUILD_TOOLS=Off
          -DLLVM_ENABLE_LTO=Off
          -DLLVM_TARGETS_TO_BUILD=X86
          -DLLVM_ENABLE_DOXYGEN=Off
          -G "Unix Makefiles" ../llvm
  COMMAND cd ${LLVM_DIR}/build && make -j${NJOB}
)

add_custom_target(llvm
  COMMAND cd ${LLVM_DIR}/build && make -j${NJOB}
)

add_subdirectory(lib)
# add_subdirectory(unittest)
