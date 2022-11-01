cmake_minimum_required(VERSION 3.10)
project(compiler)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

add_executable(compiler
	src/main.cpp
	src/tokenizer.cpp
	src/parser.cpp
	src/ast.cpp
	src/tac.cpp
)
