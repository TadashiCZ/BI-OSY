cmake_minimum_required(VERSION 3.13)
project(progtest01)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -W -Wall -Wextra -pedantic")

link_directories("/home/tadashi/repos/BI-OSY/progtest01/x86_64-linux-gnu")

add_executable(progtest01 solution.cpp progtest_solver.h sample_tester.h sample_tester.cpp common.h)

target_link_libraries(progtest01 pthread progtest_solver)