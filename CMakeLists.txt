cmake_minimum_required(VERSION 3.10)
project(compiler)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

add_executable(compiler
	src/main.cpp
	src/tokenizer.cpp
	src/parser.cpp
	src/ast.cpp
	src/tac.cpp
	src/codegen.cpp
	src/jit.cpp
)
llvm_map_components_to_libnames(llvm_libs support core irreader orcjit native)
target_link_libraries(compiler ${llvm_libs})

add_executable(test
	src/test.cpp
)
target_link_libraries(test ${llvm_libs})
