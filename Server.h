//
//
// Created by mateusz on 23.05.2021.

#ifndef ZADANIE_2_SERVER_H
#define ZADANIE_2_SERVER_H

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
#include "Connection.h"
#include "DataBuilders.h"

class MsgQueue {
    static constexpr int CLIENTS_SIZE = 25;
    static constexpr int NO_EVENT = -1;
    Client clients[CLIENTS_SIZE];
    int64_t expectedEventNo[CLIENTS_SIZE];
    size_t nextClientIndex;
    uint32_t eventsCount;

    static void advanceModulo(size_t &i) {
        i++;
        i %= CLIENTS_SIZE;
    }
public:
    class NoClientWithMessagesException : std::exception {
        const char * what() const noexcept override {
            return "No Client with messages Exeption!";
        }
    };

    class InvalidEventNoException : std::exception {
        const char * what() const noexcept override {
            return "Invalid event No!";
        }
    };

    MsgQueue() {
        eventsCount = 0;
        nextClientIndex = 0;
        for (int i = 0; i < CLIENTS_SIZE; ++i) {
            expectedEventNo[i] = NO_EVENT;
        }
    }

    bool addClient(Client client, uint32_t nextExpectedEventNo);

    bool hasMessages() const;

    size_t getNextExpectedClientIndexAndAdvance();

    uint32_t getExpectedEventNo(size_t index) const {
        return expectedEventNo[index];
    }

    const Client &getClientByIndex(size_t index);

    void setEventsCount(uint32_t events) {
        eventsCount = events;
    }

    void setExpectedEventsToZero();

    void updateClient(const Client &client, uint32_t nextExpectedEventNo);

    void deleteClientFromQueue(const Client &client);

    void setExpectedEventNo(size_t index, uint32_t value);
};

class ClientManager {
    using clientValue_t = std::pair<Player, int>;
    std::map<Client, clientValue_t> clients;
    Game game;
    int countReadyPlayers;
    int countNotObservers;
    Random random;
    std::vector<Record> gameRecords;
    MsgQueue queue;
    static constexpr int PLAYER_INDEX = 0;
    static constexpr int TICKS_INDEX = 1;
    inline static const std::string TAG = "Client Manager: ";

public:
    ClientManager(uint32_t width, uint32_t height, int turningSpeed, uint32_t seed) : game(width, height, turningSpeed),
                                                                                      countReadyPlayers(0),
                                                                                      countNotObservers(0),
                                                                                      random(seed) {};

    // Gets package containing events that next client in queue expects.
    std::pair<WritePacket, Client> handleNextInQueue();

    void addTicksAndCleanInactive() {
        for (auto it = clients.begin(), next_it = it; it != clients.end(); it = next_it) {
            ++next_it;
            auto &valIt = it->second;
            std::get<TICKS_INDEX>(valIt)++;
            if (std::get<TICKS_INDEX>(valIt) >= Utils::NUMBER_OF_TICKS) {
                debug_out_1 << "TICKS: Erasing client " << it->first << std::endl;
                removeClient(it);
                debug_out_1 << "TICKS: Clients size after = " << clients.size() << std::endl;
            }
        }
    }

    Game &getGame() {
        return game;
    }

    bool canGameStart() const {
        debug_out_1 << "cout not observers " << countNotObservers << " count ready players" << countReadyPlayers << std::endl;
        return !game.isGameNow() && countReadyPlayers == countNotObservers && countNotObservers >= 2;
    }

    bool isNameUnique(const std::string &name, const Client &c) {
        for (const auto &it : clients) {
            if (it.second.first.getName() == name && !(it.first == c))
                return false;
        }
        return true;
    }

    void addClient(const Client &client, const ClientMessage &newMsg);

    void removeClient(const std::map<Client, clientValue_t>::iterator &iterator);

    void updateClientPlayersInfo(const Client &client, clientValue_t &val, const ClientMessage &message);

    void handleClient(Client &newClient, const ClientMessage &newMsg);

     bool clientExists(const Client &client) const {
        return clients.find(client) != clients.end();
    }

     bool hasMessages() const {
        return queue.hasMessages();
    }

    auto startGame();

    void endGame();

    void performRound();

    MsgQueue& getQueue() {
        return queue;
    }
};

class Server {
    inline static const std::string TAG = "Server: ";
    ServerData serverData;
    ClientManager manager;
    UDPServerSocket serverSocket;
    PollServer pollServer;

    static void readTimeout(int fd);

    void readFromClients(struct sockaddr_storage &clientAddr, ReadPacket &packet);

    void sendPacketToClient(const Client &client, const WritePacket &writePacket) const;

public:
    explicit Server(const ServerData &serverData) : serverData(serverData),
                                                    manager(serverData.getWidth(), serverData.getHeight(),
                                                            serverData.getTuriningSpeed(), serverData.getSeed()),
                                                    serverSocket(serverData.getPortNum()),
                                                    pollServer(serverSocket.getSocket(),
                                                               serverData.getRoundsPerSec()) {}

    [[noreturn]] void start();
};

#endif //ZADANIE_2_SERVER_H
