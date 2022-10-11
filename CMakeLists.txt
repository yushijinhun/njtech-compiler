cmake_minimum_required(VERSION 3.10)
project(compiler)
add_compile_options(-Wall -Wextra)

add_executable(test_tokenizer test_tokenizer.cpp tokenizer.cpp)
