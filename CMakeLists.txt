cmake_minimum_required(VERSION 3.30)
project(BigNum)
set(CMAKE_CXX_STANDARD 20)
add_executable(BigNum main.cpp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
message("CMAKE_CXX_FLAGS = ${CMAKE_CXX_FLAGS}")

