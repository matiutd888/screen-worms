cmake_minimum_required(VERSION 3.19)
project(zadanie_2)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-g -Wall")

add_executable(zadanie_2 server.cpp)

add_library(err err.c)

macro (add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err)
    endif()
endmacro()

add_executable(echo-server echo-server.c)
add_executable(echo-client echo-client.c)