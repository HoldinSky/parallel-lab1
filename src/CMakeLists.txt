# how to run in debug:
# python3 build.py {target build folder} {executable name}

cmake_minimum_required(VERSION 3.25)
project(parallel-lab1)

set(CMAKE_CXX_STANDARD 17)

include_directories(./src)

add_executable(
        lab1
        main.cpp
        resources.cpp
)
