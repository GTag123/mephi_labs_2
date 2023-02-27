cmake_minimum_required(VERSION 3.16)
project(untitled)
set(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
add_executable(untitled main.cpp)

