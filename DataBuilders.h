//
// Created by mateusz on 23.05.2021.
//

#ifndef ZADANIE_2_DATABUILDERS_H
#define ZADANIE_2_DATABUILDERS_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <set>
#include "err.h"
#include "Utils.h"
#include <iostream>

class ServerData {
    uint32_t width, height;
    int turiningSpeed;
    uint16_t portNum;
    uint32_t seed;
    int roundsPerSec;
public:
    ServerData(uint32_t width, uint32_t height, int turiningSpeed, uint16_t portNum, uint32_t seed, int roundsPerSec)
            : width(width), height(height), turiningSpeed(turiningSpeed), portNum(portNum), seed(seed),
              roundsPerSec(roundsPerSec) {}

    friend std::ostream &operator<<(std::ostream &os, const ServerData &data) {
        os << "width: " << data.width << " height: " << data.height << " turiningSpeed: " << data.turiningSpeed
           << " portNum: " << data.portNum << " seed: " << data.seed << " roundsPerSec: " << data.roundsPerSec;
        return os;
    }

     uint32_t getWidth() const {
        return width;
    }

     uint32_t getHeight() const {
        return height;
    }

     int getTuriningSpeed() const {
        return turiningSpeed;
    }

     uint16_t getPortNum() const {
        return portNum;
    }

     uint32_t getSeed() const {
        return seed;
    }

     int getRoundsPerSec() const {
        return roundsPerSec;
    }
};



class ServerDataBuilder {
    uint32_t width, height;
    int turiningSpeed;
    uint16_t portNum;
    uint32_t seed;
    int roundsPerSec;
public:
    ServerDataBuilder() {
        width = Utils::DEFAULT_BOARD_WIDTH;
        height = Utils::DEFAULT_BOARD_HEIGHT;
        turiningSpeed = Utils::DEFAULT_TURINING_SPEED;
        portNum = Utils::DEFAULT_SERVER_PORT_NUM;
        seed = time(NULL);
        roundsPerSec = Utils::DEFAULT_ROUNDS_PER_SEC;
    }

    void parse(int argc, char **argv);

     ServerData build() const {
        return ServerData(width, height, turiningSpeed, portNum, seed, roundsPerSec);
    }
};


class ClientData {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
public:
    ClientData(const std::string &serverAddress, const std::string &playerName, uint16_t serverPortNum,
               const std::string &guiAddress, uint16_t guiPortNum) : serverAddress(serverAddress),
                                                                     playerName(playerName),
                                                                     serverPortNum(serverPortNum),
                                                                     guiAddress(guiAddress), guiPortNum(guiPortNum) {}


    friend std::ostream &operator<<(std::ostream &os, const ClientData &data) {
        os << "serverAddress: " << data.serverAddress << " playerName: " << data.playerName << " serverPortNum: "
           << data.serverPortNum << " guiAddress: " << data.guiAddress << " guiPortNum: " << data.guiPortNum;
        return os;
    }

     const std::string &getServerAddress() const {
        return serverAddress;
    }

     const std::string &getPlayerName() const {
        return playerName;
    }

     uint16_t getServerPortNum() const {
        return serverPortNum;
    }

     const std::string &getGuiAddress() const {
        return guiAddress;
    }

     uint16_t getGuiPortNum() const {
        return guiPortNum;
    }
};


class ClientDataBuilder {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
    uint32_t next_expected_event_no;
public:
    ClientDataBuilder() {
        serverAddress = "";
        playerName = "";
        serverPortNum = Utils::DEFAULT_SERVER_PORT_NUM;
        guiAddress = Utils::DEFAULT_GUI_SERVER_NAME;
        guiPortNum = Utils::DEFAULT_GUI_PORT_NUM;
    }

    void parse(int argc, char **argv);

    ClientData build() const {
        return ClientData(serverAddress, playerName, serverPortNum, guiAddress, guiPortNum);
    }
};
#endif //ZADANIE_2_DATABUILDERS_H
