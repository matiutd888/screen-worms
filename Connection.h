//
// Created by mateusz on 19.05.2021.
//

#ifndef ZADANIE_2_Server_H
#define ZADANIE_2_Server_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstring>
#include <unistd.h>
#include <map>
#include <set>
#include "err.h"
#include "PollUtils.h"
#include "Game.h"
#include "Packet.h"
#include <iostream>

enum IPaddressType {
    IPv4 = 0, IPv6 = 1
};

class Connection {
    int clientSock;

public:
    Connection() = default;

    Connection(uint16_t portNum) {
        int sockfd;
        struct addrinfo hints, *servinfo, *p = NULL;

        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE; // use my IP

        if (getaddrinfo(NULL, std::to_string(portNum).c_str(), &hints, &servinfo) != 0) {
            syserr("getaddrinfo");
        }

        // loop through all the results and bind to the first we can
        for (p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                                 p->ai_protocol)) == -1) {
                perror("listener: socket");
                continue;
            }

            if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("listener: bind");
                continue;
            }
            break;
        }

        if (p == NULL)
            syserr("listener: failed to bind socket\n");

        clientSock = sockfd;

        freeaddrinfo(servinfo);
    }


    int getSocket() const {
        return clientSock;
    }

    void doClose() {
        if (close(clientSock) != 0)
            syserr("Close error!");
    }

    ~Connection() {
        // doClose(); TODO chyba będę to robił dwa razy.
    }
};

// get sockaddr, IPv4 or IPv6:

inline void *getInAddr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *) sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *) sa)->sin6_addr);
}

class Client {
    uint16_t portNum;
    std::string ipAddress;
    IPaddressType ipType;
    uint64_t sessionID;

public:

    Client(struct sockaddr *s, uint64_t sessionID) : Client(s) {
        this->sessionID = sessionID;
    };

    // We do not compare session ID.
    bool operator<(const Client &rhs) const {
        if (ipType != rhs.ipType)
            return ipType < rhs.ipType;

        if (ipAddress != rhs.ipAddress)
            return ipAddress < rhs.ipAddress;

        if (portNum != rhs.portNum)
            return portNum < rhs.portNum;

        return false;
    }

    bool operator==(const Client &rhs) const {
        return ipType == rhs.ipType && ipAddress == rhs.ipAddress && portNum == rhs.portNum;
    }

    void setSessionID(uint64_t sessionID) {
        this->sessionID = sessionID;
    }

    uint64_t getSessionID() const {
        return sessionID;
    }

    [[nodiscard]] std::string toString() const {
        return "Client(" + ipAddress + ", " + std::to_string(portNum) + ")";
    }

    Client() = default;

    explicit Client(struct sockaddr *s) {
        if (s->sa_family == AF_INET) {
            ipType = IPaddressType::IPv4;
            portNum = ((struct sockaddr_in *) s)->sin_port;
            std::cout << "IP TYPE = IPV4" << std::endl;
        } else if (s->sa_family == AF_INET6) {
            ipType = IPaddressType::IPv6;
            portNum = ((struct sockaddr_in6 *) s)->sin6_port;
            std::cout << "IP TYPE = IPV6" << std::endl;
        } else {
            syserr("Client: Incorrect sa_family!");
        }

        char buffIP[INET6_ADDRSTRLEN];
        if (inet_ntop(s->sa_family, getInAddr(s), buffIP, sizeof(buffIP)) == NULL) {
            syserr("Client: Error during inet_ntop!");
        }

        ipAddress = std::string(buffIP);
    }
};

class ClientManager {
    std::set<Client> clients;
    std::map<Client, int> countTicks;

public:
    ClientManager() = default;

    void addTicksAndCleanInactive() {
        auto it = countTicks.begin();
        while (it != countTicks.end()) {
            it->second++;
            if (it->second == 2) {
                // supported in C++11
                std::cout << "Erasing client " << it->first.toString() << std::endl;
                std::cout << "Clients size before = " << clients.size() << std::endl;
                clients.erase(it->first); // Erasing client.
                std::cout << "Clients size after = " << clients.size() << std::endl;
                it = countTicks.erase(it);
            } else {
                ++it;
            }
        }
    }

    void insertClient(const Client &client) {
        clients.insert(client);
        countTicks[client] = 0;
    }

    size_t getClientsCount() const {
        return clients.size();
    }
};

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

    [[nodiscard]] uint16_t getPortNum() const {
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
        portNum = Utils::DEFAULT_PORT_NUM;
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

class Server {
    inline static const std::string TAG = "Server: ";

    ClientManager manager;
    PollUtils pollUtils;
    Connection connection;
    Game game;
    ServerData serverData;
public:

    Server(const ServerData &serverData) : serverData(serverData), game(serverData.getWidth(), serverData.getHeight(),
                                                                        serverData.getTuriningSpeed()) {
        connection = Connection(serverData.getPortNum());
        pollUtils = PollUtils(connection.getSocket(), serverData.getRoundsPerSec());
    }

    [[noreturn]] void start() {
        while (true) {
            int pollCount = pollUtils.doPoll();
//            ROUND > CLIENT_TIMEOUT > MESSAGES
            if (pollCount < 0) {
                std::cout << "Error in doPoll!" << std::endl;
                continue;
            }
            if (pollCount == 0) {
                std::cout << "No DoPol events!" << std::endl;
            }
            if (pollUtils.hasPollinOccurred(PollUtils::TIMEOUT_ROUND)) {
//                std::cout << TAG<< "Timeout ROUND tick!" << std::endl;
                uint64_t exp;
                ssize_t s = read(pollUtils.getDescriptor(PollUtils::TIMEOUT_ROUND), &exp, sizeof(uint64_t));
                if (s != sizeof(uint64_t))
                    perror("read timeout round");
            }
            if (pollUtils.hasPollinOccurred(PollUtils::TIMEOUT_CLIENT)) {
                std::cout << TAG << "Timeout CLIENT tick" << std::endl;
                manager.addTicksAndCleanInactive();
                uint64_t exp;
                ssize_t s = read(pollUtils.getDescriptor(PollUtils::TIMEOUT_CLIENT), &exp, sizeof(uint64_t));
                if (s != sizeof(uint64_t))
                    perror("read timeout pollutils");
            }
            if (pollUtils.hasPollinOccurred(PollUtils::MESSAGE_CLIENT)) {
                std::cout << TAG << "Client message!" << std::endl;
                struct sockaddr_storage clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);

                Packet packet;
                ssize_t numBytes = recvfrom(pollUtils.getDescriptor(PollUtils::MESSAGE_CLIENT),
                                            packet.getBuffer(),
                                            packet.getSize() - 1,
                                            0,
                                            reinterpret_cast<sockaddr *>(&clientAddr),
                                            &clientAddrLen);


                if (numBytes < 0) {
                    packet.errorOccured();
                    std::cout << TAG << "ERROR READING FROM CLIENT " << std::endl;
                    continue;
                }

                char *buff = packet.getBuffer();
                buff[packet.getSize() - 1] = '\0';
                printf("message: %s\n", buff);

                if (clientAddr.ss_family == AF_INET6) {
                    std::cout << "CLIENT USES IPv6 \n";
                } else {
                    std::cout << "CLIENT USES IPv4 \n";
                }

                Client client(reinterpret_cast<sockaddr *>(&clientAddr));
                std::cout << TAG << "Putting client " << client.toString() << " to map!" << std::endl;
                manager.insertClient(client);
                std::cout << "Client count " << manager.getClientsCount() << std::endl;
            }
        }
    }
};

#endif //ZADANIE_2_Server_H
