cmake_minimum_required(VERSION 3.19)
project(zadanie_2 C CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_C_FLAGS "-g -Wall")



macro (add_executable _name)
    _add_executable(${ARGV})
    if (TARGET ${_name})
        target_link_libraries(${_name} err)
    endif()
endmacro()

add_executable(echo-server examples/echo-server-udp.c)
add_executable(test examples/test.cpp)
add_executable(echo-client examples/echo-client-udp.c)
add_executable(beej-listener examples/listener.c)
add_executable(beej-talker examples/talker.c)
add_executable(beej-poll-erver examples/poll_server_beej.c)

add_library(err err.cpp err.h)
add_library(Utils Utils.cpp Utils.h)
add_library(Random Random.cpp Random.h)
add_library(Packet Packet.cpp Packet.h)
add_library(Board Game.cpp Game.h)
add_library(Connection Connection.cpp Connection.h)
add_library(Messages Messages.cpp Messages.h)
add_executable(serwer main.cpp)



#add_executable(zadanie_2 err.h err.c main.cpp Connection.cpp Connection.h Board.cpp Board.h Utils.cpp Utils.h PollUtils.h PollUtils.cpp Messages.cpp Messages.h Packet.cpp Packet.h)
