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
#include <queue>
#include "Connection.h"
#include "DataBuilders.h"

class ClientManager {
    using clientValue_t = std::tuple<Player, int, uint32_t>;
    std::map<Client, clientValue_t> clients;
    int countReadyPlayers;
    int countNotObservers;
    Game game;
    std::vector<Record> gameRecords;
    Random random;
    static constexpr int PLAYER_INDEX = 0;
    static constexpr int NEXT_EXPECTED_EVENT_NO = 2;
    static constexpr int TICKS_INDEX = 1;
    std::queue<Client> roundRobin;
public:
    ClientManager(uint32_t width, uint32_t height, int turningSpeed, uint32_t seed) : game(width, height, turningSpeed),
                                                                       countReadyPlayers(0), countNotObservers(0),
                                                                                      random(seed) {};

    std::vector<Record> getRecords(uint32_t next_expected_event_no) {
        return std::vector<Record>(gameRecords.begin() + next_expected_event_no, gameRecords.end());
    }

    size_t fillPacket();

    std::vector<WritePacket> constructPackets(const std::vector<Record> &records) {
        size_t it = 0;
        std::vector<WritePacket> packets;
        while (it < records.size()) {
            WritePacket packet;
            ServerMessage::startServerMessage(packet, game.getGameId());
            bool fits = true;
            while (fits && it < records.size()) {
                try {
                    records[it].encode(packet);
                    it++;
                } catch (const Packet::PacketToSmallException &p) {
                    fits = false;
                }
            }
            packets.push_back(packet);
        }
        return packets;
    }

    void sendPacketToClient() {

    }

    void addTicksAndCleanInactive() {
        for (auto it = clients.cbegin(), next_it = it; it != clients.cend(); it = next_it) {
            ++next_it;
            auto &valIt = it->second;
            if (std::get<TICKS_INDEX>(valIt) == 2) {
                debug_out_1 << "TICKS: Erasing client " << it->first << std::endl;
                clients.erase(it);
                debug_out_1 << "TICKS: Clients size after = " << clients.size() << std::endl;
            }
        }
    }

    Game &getGame() {
        return game;
    }

    bool canGameStart() const {
        return !game.isGameNow() && countReadyPlayers == countNotObservers && countNotObservers >= 2;
    }

    // 1. Check if exists
    // 2.1 If doesnt exist, check if it exceeds the limit.
    // 2.1.1 If if exceedes the limit, ignore the client.
    // 2.1.2 If it doesnt exceed the limit, add the client (go to A)
    // 2.2 If it exists, check if session id is bigger, smaller or the same.
    // 2.2.1 If its smaller, ignore
    // 2.2.2 If its bigger, perform client remove and client add
    // 2.2.3 If its the same, go to 3
    // 3 Check if playername is same as before.r
    // 3.1 If its the same, update the player.
    // 3.2 If its different perform R and A.
    // 4. Check for game start.

// R. Client remove:
    // A. Client add:
    void addClient(const Client &client, const ClientMessage &newMsg) {
        // newMsg.
        Player player(newMsg.getPlayerName(), static_cast<TurnDirection>(newMsg.getTurnDirection()),
                      newMsg.getSessionId());
        if (!player.isObserver()) {
            countNotObservers++;
            if (!game.isGameNow()) {
                if (player.isPlayerReady())
                    countReadyPlayers++;
            }
        }
        clients[client] = std::make_tuple(player, 0, newMsg.getNextExpectedEventNo());
    }

    void removeClient(const std::map<Client, clientValue_t>::iterator &iterator) {
        const Client &client = iterator->first;
        const Player &player = std::get<PLAYER_INDEX>(iterator->second);
        if (!player.isObserver())
            countNotObservers--;
        if (!game.isGameNow()) {
            if (player.isPlayerReady())
                countReadyPlayers--;
        }
        clients.erase(iterator);
    }

    void updateClientPlayersInfo(clientValue_t &val, const ClientMessage &message) {
        Player &player = std::get<PLAYER_INDEX>(val);
        bool before = player.isPlayerReady();
        player.setDirection(static_cast<TurnDirection>(message.getTurnDirection()));
        if (game.isGameNow()) {
            game.updatePlayer(player);
        } else {
            bool after = player.isPlayerReady();
            if (!before && after)
                countReadyPlayers++;
        }
        std::get<TICKS_INDEX>(val) = 0;
        std::get<NEXT_EXPECTED_EVENT_NO>(val) = message.getNextExpectedEventNo();
    }

    void handleClient(Client &newClient, const ClientMessage &newMsg) {
        if (!clientExists(newClient)) {
            if (clients.size() + 1 > Utils::GAME_CLIENTS_LIMIT)
                return;

            addClient(newClient, newMsg);
            return;
        }
        const auto &it = clients.find(newClient);
        clientValue_t &clientValue = it->second;
        std::get<TICKS_INDEX>(clientValue) = 0;
        const Client &mapClient = it->first;
        if (std::get<PLAYER_INDEX>(clientValue).getSessionId() < newMsg.getSessionId())
            return;
        if (std::get<PLAYER_INDEX>(clientValue).getSessionId() > newMsg.getSessionId()) {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
        if (std::get<PLAYER_INDEX>(clientValue).getName() == newMsg.getPlayerName()) {
            updateClientPlayersInfo(clientValue, newMsg);
        } else {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
    }

    bool clientExists(const Client &client) const {
        return clients.find(client) != clients.end();
    }


    [[nodiscard]] size_t getClientsCount() const {
        return clients.size();
    }

    auto startGame() {
        gameRecords.clear();
        std::vector<Player> gamePlayers;
        for (const auto &it : clients) {
            const auto &clientVal = it.second;
            if (!clientVal.first.isObserver()) {
                gamePlayers.push_back(clientVal.first);
            }
        }
        std::vector<Record> records = game.startNewGame(gamePlayers, random);
        gameRecords.insert(gameRecords.end(), records.begin(), records.end());
        return records;
    }

    void sendToAll(const std::vector<WritePacket> &vector) {
        for (const auto &client : clients) {

        }
    }
};

class Server {
    inline static const std::string TAG = "Server: ";
    ClientManager manager;
    UDPServerSocket clientSocket;
    PollServer pollServer;
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
            throw Packet::FatalDecodingException();
        }
        packet.setSize(numBytes);
    }

public:

    Server(const ServerData &serverData) : serverData(serverData),
                                           manager(serverData.getWidth(), serverData.getHeight(),
                                                   serverData.getTuriningSpeed(), serverData.getSeed()),
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
                    manager.handleClient(client, clientMessage);
                    if (!manager.getGame().isGameNow()) {
                        if (manager.canGameStart()) {
                            manager.startGame();
                            manager.sendToAll(manager.constructPackets(manager.getRecords(0)));

                        } else {
                            debug_out_1 << "Game cannot start!" << std::endl;
                        }
                    }


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
