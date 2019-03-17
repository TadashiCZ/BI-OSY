cmake_minimum_required(VERSION 3.13)
project(cviceni02)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "-pthread")
add_executable(cviceni02 condvar01.cpp condvar_pt01.c prodcons_cv_01.cpp)