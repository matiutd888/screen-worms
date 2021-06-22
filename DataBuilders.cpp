//
// Created by mateusz on 23.05.2021.
//

#include <cassert>
#include "DataBuilders.h"


static void assertm(bool exp, const char *msg)  {
    if (!exp)
        syserr(msg);
}

static uint64_t converter(const char *arg, uint64_t lowerBound, uint64_t upperBound, const char *errorMsg) {
    size_t size = 0;
    uint64_t argumentNumber = 0;
    try {
        argumentNumber = std::stoul(arg, &size);
    } catch (...) {
        syserr(errorMsg);
    }
    if (size != std::string(arg).size())
        syserr(errorMsg);
    if (argumentNumber > upperBound || argumentNumber < lowerBound) {
        syserr(errorMsg);
    }

    return argumentNumber;
}

void ServerDataBuilder::parse(int argc, char **argv) {
    char c;
    while ((c = getopt(argc, argv, Utils::optstring)) != -1) {
        switch (c) {
            case 'p':
                portNum = converter(optarg, 1, UINT16_MAX, "Invalid portnum argument!");
                break;
            case 's':
                seed = converter(optarg, 1, UINT32_MAX, "Invalid seed argument!");
                break;
            case 't':
                turiningSpeed = int(converter(optarg, 1, 90, "Invalid turiningSpeed argument!"));
                break;
            case 'v':
                roundsPerSec = int(converter(optarg,1, 250, "Invalid roundsPerSec argument!"));
                break;
            case 'w':
                width = converter(optarg, 16, 2048, "Invalid width argument!");
                break;
            case 'h':
                height = converter(optarg, 16, 2048, "Invalid height argument!");
                break;
            default:
                syserr("Incorrect argument!\n");
        }
    }
    if (optind < argc) {
        syserr("Unexpected argument!\n");
    }
}

static void checkPlayerName(const std::string &name) {
    for (const auto &c : name) {
        assertm(c >= 33 && c <= 126, "Invalid Player Name!");
    }
}

void ClientDataBuilder::parse(int argc, char **argv) {
    char c;
    if (argc < 2)
        syserr("Client usage: game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
    serverAddress = argv[1];
    optind = 2;
    while ((c = getopt(argc, argv, "n:p:i:r:")) != -1) {
        std::string argStr(optarg);
        printf("%c\n", c);
        switch (c) {
            case 'n':
                playerName = argStr;
                checkPlayerName(playerName);
                break;
            case 'p':
                serverPortNum = converter(optarg, 1, UINT16_MAX, "Invalid Server portnum!");
                break;
            case 'i':
                guiAddress = argStr;
                break;
            case 'r':
                guiPortNum = converter(optarg, 1, UINT16_MAX, "Invalid Server portnum!");
                break;
            default:;
        }
    }
    if (optind < argc) {
        syserr("Unexpected argument!\n");
    }
}
