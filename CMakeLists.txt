cmake_minimum_required(VERSION 3.5)
project(bylSocket)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(test)