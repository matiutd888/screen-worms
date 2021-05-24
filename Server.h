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

class ClientManager {
    using clientValue_t = std::pair<Player, int>;
    std::map<Client, clientValue_t> clients;
    int countReadyPlayers;
    int countNotObservers;
public:
    ClientManager() = default;

    void addTicksAndCleanInactive() {
        for (auto it = clients.cbegin(), next_it = it; it != clients.cend(); it = next_it) {
            ++next_it;
            auto &valIt = it->second;
            if (valIt.second == 2) {
                debug_out_1 << "TICKS: Erasing client " << it->first << std::endl;
                clients.erase(it);
                debug_out_1 << "TICKS: Clients size after = " << clients.size() << std::endl;
            }
        }
    }

    // 1. Check if exists
    // 2.1 If doesnt exist, check if it exceeds the limit.
    // 2.1.1 If if exceedes the limit, ignore the client.
    // 2.1.2 If it doesnt exceed the limit, add the client (go to A)
    // 2.2 If it exists, check if session id is bigger, smaller or the same.
    // 2.2.1 If its smaller, ignore
    // 2.2.2 If its bigger, perform client remove and client add
    // 2.2.3 If its the same, go to 3
    // 3 Check if playername is same as before.
    // 3.1 If its the same, update the player.
    // 3.2 If its different perform R and A.
    // 4. Check for game start.


// R. Client remove:
    // A. Client add:


    void addClient(const Client &client, const ClientMessage &newMsg) {
        // TODO
        // PAMIĘTAĆ o ustawienuu sessionID z newMSg.
    }

    void removeClient(const std::map<Client, clientValue_t>::iterator &iterator) {
        // TODO
    }

    void updateClientsInfo(clientValue_t &val, const ClientMessage &message) {
        val.first.setDirection(static_cast<TurnDirection>(message.getTurnDirection()));
        // TODO
    }

    void handleClient(const Client &newClient, const ClientMessage &newMsg) {
        // TODO ustawiać ticki na 0.
        if (!clientExists(newClient)) {
            if (clients.size() + 1 > Utils::GAME_CLIENTS_LIMIT)
                return;

            addClient(newClient, newMsg);
            return;
        }
        const auto &it = clients.find(newClient);
        clientValue_t &clientValue = it->second;
        const Client &mapClient = it->first;
        if (newClient.getSessionID() < newMsg.getSessionId())
            return;
        if (newClient.getSessionID() > newMsg.getSessionId()) {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
        if (clientValue.first.getName() == newMsg.getPlayerName()) {
            updateClientsInfo(clientValue, newMsg);
        } else {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
    }

    bool clientExists(const Client &client) {
        return clients.find(client) != clients.end();
    }

    [[nodiscard]] size_t getClientsCount() const {
        return clients.size();
    }
};

class Server {
    inline static const std::string TAG = "Server: ";

    ClientManager manager;
    UDPServerSocket clientSocket;
    PollServer pollServer;
    Game game;
    ServerData serverData;


    void readTimeout(int fd) {
        uint64_t exp;
        ssize_t s = read(fd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t))
            perror("read timeout pollutils");
    }

    void readFromClients(struct sockaddr_storage &clientAddr, ReadPacket &packet) {

        socklen_t clientAddrLen = sizeof(clientAddr);

        ssize_t numBytes = recvfrom(pollServer.getDescriptor(PollServer::MESSAGE_CLIENT),
                                    packet.getBuffer(),
                                    packet.getMaxSize(),
                                    0,
                                    reinterpret_cast<sockaddr *>(&clientAddr),
                                    &clientAddrLen);


        if (numBytes < 0) {
            packet.errorOccured();
            debug_out_1 << TAG << "ERROR READING FROM CLIENT " << std::endl;
            throw Utils::ReadException();
        }
        packet.setSize(numBytes);
    }
public:

    Server(const ServerData &serverData) : serverData(serverData),
                                           game(serverData.getWidth(), serverData.getHeight(),
                                                serverData.getTuriningSpeed()),
                                           clientSocket(serverData.getPortNum()),
                                           pollServer(clientSocket.getSocket(), serverData.getRoundsPerSec()) {}

    [[noreturn]] void start() {
        while (true) {
            int pollCount = pollServer.doPoll();
            if (pollCount < 0) {
                debug_out_1 << "Error in doPoll!" << std::endl;
                continue;
            }
            if (pollCount == 0) {
                debug_out_1 << "No DoPol events!" << std::endl;
            }
            if (pollServer.hasPollinOccurred(PollServer::TIMEOUT_ROUND)) {
                readTimeout(pollServer.getDescriptor(PollServer::TIMEOUT_ROUND));
            }
            if (pollServer.hasPollinOccurred(PollServer::TIMEOUT_CLIENT)) {
                debug_out_1 << TAG << "Timeout CLIENT tick" << std::endl;
                manager.addTicksAndCleanInactive();
                readTimeout(pollServer.getDescriptor(PollServer::TIMEOUT_CLIENT));
            }
            if (pollServer.hasPollinOccurred(PollServer::MESSAGE_CLIENT)) {
                debug_out_1 << TAG << "Client message!" << std::endl;

                try {
                    struct sockaddr_storage clientAddr;
                    ReadPacket packet;
                    readFromClients(clientAddr, packet);
                    ClientMessage clientMessage = ClientMessage::decode(packet);
                    debug_out_1 << clientMessage << std::endl;
                    Client client(clientAddr);
//                    manager.handleClient(client, clientMessage);
                    // TODO
//                    debug_out_1 << TAG << "Putting client " << client << " to map!" << std::endl;
//                    manager.insertClient(client);
//                    debug_out_1 << "Client count " << manager.getClientsCount() << std::endl;
                } catch (...) {
                    debug_out_0 << "Error decoding client message!" << std::endl;
                    continue;
                }
            }
        }
    }
};

#endif //ZADANIE_2_SERVER_H
