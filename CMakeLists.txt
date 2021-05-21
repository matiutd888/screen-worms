cmake_minimum_required(VERSION 3.19)
project(zadanie_2)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-g -Wall")

add_executable(zadanie_2 main.cpp server.cpp server.h board.cpp board.h Utils.cpp Utils.h PollUtils.h PollUtils.cpp)

add_library(err err.c)

macro (add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err)
    endif()
endmacro()

add_executable(echo-server examples/echo-server-udp.c)
add_executable(echo-client examples/echo-client-udp.c)
add_executable(beej-listener examples/listener.c)
add_executable(beej-talker examples/talker.c)
add_executable(beej-poll-erver examples/poll_server_beej.c)