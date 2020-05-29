cmake_minimum_required(VERSION 3.14)
project(here)

set(CMAKE_CXX_STANDARD 14)
find_package (Threads)

add_executable(ex3 Barrier.cpp MapReduceFramework.cpp)
target_link_libraries (ex3 ${CMAKE_THREAD_LIBS_INIT})