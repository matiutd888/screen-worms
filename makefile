OBJS	= Server.o Connection.o DataBuilders.o Game.o PollUtils.o Packet.o Messages.o Random.o err.o
SOURCE	= serwer.cpp Server.cpp Connection.c DataBuilders.cpp Game.cpp Messages.	cpp PollUtils.cpp Packet.cpp Random.cpp err.cpp
HEADER	= Server.h Connection.h DataBuilders.h Game.h PollUtils.h Packet.h Messages.h Random.h Utils.h err.h
OUT	= screen-worms-server screen-worms-client
CC	 = g++
FLAGS	 = -Wall -std=c++17 -c
LFLAGS	 =

screen-worms-client: $(OBJS) client.o
	$(CC) -g $(OBJS) client.o -o screen-worms-client$(LFLAGS)
screen-worms-server: $(OBJS) serwer.o
	$(CC) -g $(OBJS) serwer.o -o screen-worms-server $(LFLAGS)

serwer.o: serwer.cpp Server.o DataBuilders.o Utils.h Messages.o
	$(CC) $(FLAGS) serwer.cpp

client.o: client.cpp Connection.o
	$(CC) $(FLAGS) client.cpp

#include "err.h"
#include "PollUtils.h"
#include "Game.h"
#include "Packet.h"
#include <iostream>
#include <queue>
#include "Connection.h"
#include "DataBuilders.h"
Server.o: Server.cpp Server.h err.o PollUtils.o Packet.o Game.o Connection.o DataBuilders.o Messages.o
	$(CC) $(FLAGS) $<

#include "err.h"
#include "PollUtils.h"
#include "Game.h"
#include "Packet.h"
Connection.o: Connection.cpp Connection.h Game.o PollUtils.o err.o Packet.o
	$(CC) $(FLAGS) $<

DataBuilders.o: DataBuilders.cpp DataBuilders.h err.o Utils.h
	$(CC) $(FLAGS) $<

Game.o: Game.cpp Game.h Random.o Messages.o
	$(CC) $(FLAGS) $<

Messages.o: Messages.cpp Messages.h err.o Utils.h Packet.o
	$(CC) $(FLAGS) $<

PollUtils.o: PollUtils.cpp PollUtils.h err.o Utils.h
	$(CC) $(FLAGS) $<

Packet.o: Packet.cpp Packet.h err.o Utils.h
	$(CC) $(FLAGS) $<

Random.o: Random.cpp Random.h
	$(CC) $(FLAGS) $<

#Utils.o: Utils.h
#	$(CC) $(FLAGS) $<

err.o: err.cpp err.h
	$(CC) $(FLAGS) $<


clean:
	rm -f *.o $(OUT)