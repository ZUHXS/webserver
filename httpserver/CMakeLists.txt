cmake_minimum_required(VERSION 3.10)
project(httpserver)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp http.cpp http.h)
add_executable(httpserver ${SOURCE_FILES})