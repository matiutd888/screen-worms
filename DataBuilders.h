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

    [[nodiscard]] uint32_t getWidth() const {
        return width;
    }

    [[nodiscard]] uint32_t getHeight() const {
        return height;
    }

    [[nodiscard]] int getTuriningSpeed() const {
        return turiningSpeed;
    }

    [[nodiscard]] uint16_t getPortNum() const {
        return portNum;
    }

    [[nodiscard]] uint32_t getSeed() const {
        return seed;
    }

    [[nodiscard]] int getRoundsPerSec() const {
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

    void parse(int argc, char **argv) {
        char c;
        while ((c = getopt(argc, argv, Utils::optstring)) != -1) {
            size_t convertedSize = 0;
            std::string argStr = optarg;
            bool haveFound = false;
            switch (c) {
                case 'p': {
                    portNum = std::stoi(argStr, &convertedSize);
                    haveFound = true;
                    break;
                }
                case 's': {
                    seed = std::stoul(argStr, &convertedSize);
                    haveFound = true;
                    break;
                }
                case 't': {
                    turiningSpeed = std::stoi(argStr, &convertedSize);
                    haveFound = true;
                    break;
                }
                case 'v':
                    roundsPerSec = std::stoi(argStr, &convertedSize);
                    haveFound = true;
                    break;
                case 'w':
                    width = std::stoul(argStr, &convertedSize);
                    haveFound = true;
                    break;
                case 'h':
                    height = std::stoul(argStr, &convertedSize);
                    haveFound = true;
                    break;
                default:;
            }
            if (haveFound) {
                if (convertedSize != argStr.size())
                    syserr("Argument parsing error!");
            }
        }
    }

    ServerData build() const {
        return ServerData(width, height, turiningSpeed, portNum, seed, roundsPerSec);
    }
};


#endif //ZADANIE_2_DATABUILDERS_H
