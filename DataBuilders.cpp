//
// Created by mateusz on 23.05.2021.
//

#include "DataBuilders.h"

void ServerDataBuilder::parse(int argc, char **argv) {
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

void ClientDataBuilder::parse(int argc, char **argv) {
    char c;
    if (argc < 2)
        syserr("Client usage: game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
    serverAddress = argv[1];
    optind = 2;
    while ((c = getopt(argc, argv, "n:p:i:r:")) != -1) {
        // TODO wywalać się jak nie poda
        std::string argStr(optarg);
        size_t convertedSize = 0;
        printf("%c\n", c);
        bool haveFoundNumber = false;
        switch (c) {
            case 'n':
                playerName = argStr;
                break;
            case 'p':
                serverPortNum = std::stoi(argStr, &convertedSize);
                haveFoundNumber = true;
                break;
            case 'i':
                guiAddress = argStr;
                break;
            case 'r':
                guiPortNum = std::stoi(argStr, &convertedSize);
                haveFoundNumber = true;
                break;
            default:;
        }
        if (haveFoundNumber) {
            if (convertedSize != argStr.size())
                syserr("Argument parsing error!");
        }
    }
}
