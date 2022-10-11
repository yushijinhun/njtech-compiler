cmake_minimum_required(VERSION 3.10)
project(compiler)

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra)

add_executable(test_tokenizer
	src/test_tokenizer.cpp
	src/tokenizer.cpp
)
add_executable(homework1
	src/homework1.cpp
	src/tokenizer.cpp
)
