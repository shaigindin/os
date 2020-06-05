cmake_minimum_required(VERSION 3.14)
project(project01)

SET(CMAKE_CXX_FLAGS "-Wall -std=c++11 -funroll-loops")

set(CMAKE_CXX_STANDARD 14)

add_executable(project02 uthreads.cpp QueueWrapper.cpp Thread.cpp ./jona/test2.in.cpp)