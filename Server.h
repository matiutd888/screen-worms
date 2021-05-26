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

// TODO zastanowić się nad tym Round Robin
// Wymagałoby to tak naprawdę tylko iteratora...
// Myślę że do zrobienia, ale potem.
class MsgQueue {
    static constexpr int CLIENTS_SIZE = 25;
    static constexpr int NO_EVENT = -1;
    Client clients[CLIENTS_SIZE];
    int64_t expectedEventNo[CLIENTS_SIZE];
    size_t nextClientIndex;
    uint32_t eventsCount;
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

    static void advanceModulo(size_t &i) {
        i++;
        i %= CLIENTS_SIZE;
    }

    bool addClient(Client client, uint32_t nextExpectedEventNo) {
        for (int i = 0; i < CLIENTS_SIZE; ++i) {
            if (expectedEventNo[i] == NO_EVENT) {
                debug_out_0 << "QUEUE: " << "adding client " << client << " nextExpectedEventNo" << nextExpectedEventNo << std::endl;
                expectedEventNo[i] = nextExpectedEventNo;
                clients[i] = client;
                return true;
            }
        }
        debug_out_0 << "QUEUE: " << "Failed adding client!" << client << std::endl;
        return false;
    }

    bool hasMessages() const {
        size_t it = nextClientIndex;
        do {
            if (expectedEventNo[it] != NO_EVENT && expectedEventNo[it] < eventsCount)
                return true;
            advanceModulo(it);
        } while (it != nextClientIndex);
        return false;
    }

    size_t getNextExpectedClientIndexAndAdvance() {
        size_t beggining = nextClientIndex;
        do {
            if (expectedEventNo[nextClientIndex] != NO_EVENT && expectedEventNo[nextClientIndex] < eventsCount) {
                size_t ret = nextClientIndex;
                advanceModulo(nextClientIndex);
                return ret;
            }
            advanceModulo(nextClientIndex);
        } while (nextClientIndex != beggining);
        throw NoClientWithMessagesException();
    }

    uint32_t getExpectedEventNo(size_t index) {
        return expectedEventNo[index];
    }

    const Client &getClientByIndex(size_t index) {
        if (index >= CLIENTS_SIZE)
            throw std::invalid_argument("Invalid client index!");
        if (expectedEventNo[index] == NO_EVENT)
            throw std::invalid_argument("No client at this index!");
        return clients[index];
    }

    void setEventsCount(uint32_t events) {
        eventsCount = events;
    }

    void setExpectedEventsToZero() {
        for (int i = 0; i < CLIENTS_SIZE; ++i) {
            if (expectedEventNo[i] != NO_EVENT) {
                expectedEventNo[i] = 0;
            }
        }
    }

    void updateClient(const Client &client, uint32_t nextExpectedEventNo) {
        if (nextExpectedEventNo > eventsCount) {
            return;
        }
        for(int i = 0; i < CLIENTS_SIZE; i++) {
            if (clients[i] == client) {
                std::cout << "Client: " << client << " expected event no = " << nextExpectedEventNo << std::endl;
                expectedEventNo[i] = nextExpectedEventNo;
                return;
            }
        }
        throw std::invalid_argument("No client found!\n");
    }

    void deleteClientFromQueue(const Client &client) {
        for(size_t it = 0; it < CLIENTS_SIZE; it++) {
            if (clients[it] == client) {
                clients[it] = Client();
                expectedEventNo[it] = NO_EVENT;
            }
        }
    }

    void setExpectedEventNo(size_t index, uint32_t value) {
        if (value > eventsCount) {
            throw InvalidEventNoException();
        }
        if (index >= CLIENTS_SIZE)
            throw std::invalid_argument("Invalid client index!");

        expectedEventNo[index] = value;
    }
};

class ClientManager {
    using clientValue_t = std::pair<Player, int>;
    std::map<Client, clientValue_t> clients;
    int countReadyPlayers;
    int countNotObservers;
    Game game;
    std::vector<Record> gameRecords;
    Random random;
    MsgQueue queue;
    static constexpr int PLAYER_INDEX = 0;
    static constexpr int TICKS_INDEX = 1; // Instead of pair, initially i wanted to store a
    // tuple, thats why i hardcoded the indexes.
    static constexpr int NEXT_EXPECTED_EVENT_NO = 2;
    //   std::queue<WritePacket> packetsToSend;
    inline static const std::string TAG = "Client Manager: ";
//    std::queue<std::pair<Client, WritePacket>> packetsToSend;

public:
    ClientManager(uint32_t width, uint32_t height, int turningSpeed, uint32_t seed) : game(width, height, turningSpeed),
                                                                                      countReadyPlayers(0),
                                                                                      countNotObservers(0),
                                                                                      random(seed) {};

    [[nodiscard]] std::vector<Record> getRecords(uint32_t next_expected_event_no) const {
        if (next_expected_event_no >= gameRecords.size())
            return std::vector<Record>();
        return std::vector<Record>(gameRecords.begin() + next_expected_event_no, gameRecords.end());
    }

    [[nodiscard]] std::vector<WritePacket> constructPackets(const std::vector<Record> &records) const {
        size_t it = 0;
        std::vector<WritePacket> packets;
        std::cout << "PUSHING PACKETS WITH gameID = " << game.getGameId() << std::endl;
        while (it < records.size()) {
            WritePacket packet;
            ServerMessage::startServerMessage(packet, game.getGameId());
            bool fits = true;
            while (fits && it < records.size()) {
                try {
                    if (records[it].getEventType() == ServerEventType::NEW_GAME) {
                        debug_out_1 << "NEW GAME PUSHED WITH GAMEID = " << game.getGameId() << std::endl;
                    }
                    records[it].encode(packet);
                    if (records[it].getEventType() == ServerEventType::NEW_GAME) {
                        debug_out_1 << "NEW GAME PUSHED SUCCESSFULLY!" << std::endl;
                    }
                    it++;
                } catch (const Packet::PacketToSmallException &p) {
                    fits = false;
                }
            }
            packets.push_back(packet);
        }
        return packets;
    }

//    void pushPacketsForAll(const std::vector<WritePacket> &packets) {
//        for (const auto &packet : packets) {
//            for (const auto &client: clients) {
//                packetsToSend.push(std::make_pair(client.first, packet));
//            }
//        }
//    }

    std::pair<WritePacket, Client> handleNextInQueue() {
        size_t clientIndex = queue.getNextExpectedClientIndexAndAdvance();
        uint32_t nextExpectedEventNo = queue.getExpectedEventNo(clientIndex);
        Client client = queue.getClientByIndex(clientIndex);
        WritePacket writePacket;
        ServerMessage::startServerMessage(writePacket, game.getGameId());
        uint32_t eventNoIt = nextExpectedEventNo;
        while (eventNoIt < gameRecords.size() && writePacket.getRemainingSize() > gameRecords[eventNoIt].getSize()) {
            try {
                gameRecords[eventNoIt].encode(writePacket);
                eventNoIt++;
            } catch (const Packet::PacketToSmallException &p){
                break;
            }
        }
        queue.setExpectedEventNo(clientIndex, eventNoIt);
        return {writePacket, client};
    }

    void addTicksAndCleanInactive() {
        for (auto it = clients.begin(), next_it = it; it != clients.end(); it = next_it) {
            ++next_it;
            auto &valIt = it->second;
            std::get<TICKS_INDEX>(valIt)++;
            if (std::get<TICKS_INDEX>(valIt) >= 2) {
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
//        std::cout << "cout not observers " << countNotObservers << " count ready players" << countReadyPlayers << std::endl;
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
    bool isNameUnique(const std::string &name, const Client &c) {
        for (const auto &it : clients) {
            if (it.second.first.getName() == name && !(it.first == c))
                return false;
        }
        return true;
    }

    void addClient(const Client &client, const ClientMessage &newMsg) {
        // newMsg.
        if (!isNameUnique(newMsg.getStringPlayerName(), client))
            return;

        Player player(newMsg.getStringPlayerName(), static_cast<TurnDirection>(newMsg.getTurnDirection()),
                      newMsg.getSessionId());

        std::cout << "ADDING CLIENT " << client << " , player = " << player << std::endl;
        if (!player.isObserver()) {
            countNotObservers++;
            if (!game.isGameNow()) {
                if (player.isPlayerReady()) {
                    countReadyPlayers++;
                }
            }
        }
        std::cout << "NOT OBSERVERS = " << countNotObservers << ",  countReadyPlayers = " << countReadyPlayers << std::endl;
        clients[client] = std::make_pair(player, 0);
        queue.addClient(client, newMsg.getNextExpectedEventNo());
    }

    void removeClient(const std::map<Client, clientValue_t>::iterator &iterator) {
        std::cout << "REMOVING CLIENT " << std::endl;
        const Client &client = iterator->first;
        const Player &player = std::get<PLAYER_INDEX>(iterator->second);
        if (!player.isObserver())
            countNotObservers--;
        if (!game.isGameNow()) {
            if (player.isPlayerReady())
                countReadyPlayers--;
        }
        std::cout << "BEFORE: Clients.count = " << clients.size() << " not observers =  " << countNotObservers
                  << " not ready players = " << countReadyPlayers << std::endl;
        queue.deleteClientFromQueue(iterator->first);
        clients.erase(iterator);
    }

//    void pushClientMessagesOnQueue(const Client &client, uint32_t nextExpectedEventNo) {
//        auto records = getRecords(nextExpectedEventNo);
//        if (records.empty())
//            return;
//        auto packets = constructPackets(records);
//        for (const auto &packet : packets) {
//            packetsToSend.push(std::make_pair(client, packet));
//        }
//    }

    void updateClientPlayersInfo(const Client &client, clientValue_t &val, const ClientMessage &message) {
        Player &player = std::get<PLAYER_INDEX>(val);
//        std::cout << "Updating client " << player << std::endl;
        bool before = player.isPlayerReady();
        player.setDirection(static_cast<TurnDirection>(message.getTurnDirection()));
        if (game.isGameNow()) {
            game.updatePlayer(player);
        } else {
            bool after = player.isPlayerReady();
            if (!before && after) {
                countReadyPlayers++;
                std::cout << "PLAYER BECAME READY: " << player << " COUNT READY PLAYERS = " << countReadyPlayers << std::endl;
            }
        }
        std::get<TICKS_INDEX>(val) = 0;
        std::cout << "Updating client: " << client << " queue index!" << std::endl;
        queue.updateClient(client, message.getNextExpectedEventNo());
        //        if (gameRecords.size() > message.getNextExpectedEventNo())
//            std::cout << "pushing [" << message.getNextExpectedEventNo() << ", " << gameRecords.size() << ") for client "
//                      << client << ", player = " << player << std::endl;
//
        //        std::get<NEXT_EXPECTED_EVENT_NO>(val) = message.getNextExpectedEventNo();
    }

    void handleClient(Client &newClient, const ClientMessage &newMsg) {
        if (!clientExists(newClient)) {
            if (clients.size() + 1 > Utils::GAME_CLIENTS_LIMIT)
                return;
            if (!isNameUnique(newMsg.getStringPlayerName(), newClient))
                return;
            addClient(newClient, newMsg);
            return;
        }
        const auto &it = clients.find(newClient);
        clientValue_t &clientValue = it->second;
        std::get<TICKS_INDEX>(clientValue) = 0;
        const Client &mapClient = it->first;
        if (std::get<PLAYER_INDEX>(clientValue).getSessionId() < newMsg.getSessionId()) {
            return;
        }
        if (!isNameUnique(newMsg.getStringPlayerName(), mapClient))
            return;
        if (std::get<PLAYER_INDEX>(clientValue).getSessionId() > newMsg.getSessionId()) {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
        if (std::get<PLAYER_INDEX>(clientValue).getName() == newMsg.getStringPlayerName()) {
            updateClientPlayersInfo(it->first, clientValue, newMsg);
        } else {
            removeClient(it);
            addClient(newClient, newMsg);
            return;
        }
    }

    [[nodiscard]] bool clientExists(const Client &client) const {
        return clients.find(client) != clients.end();
    }

    [[nodiscard]] bool hasMessages() const {
        return queue.hasMessages();
    }

    auto startGame() {
        queue.setExpectedEventsToZero();
        gameRecords.clear();
        std::vector<Player> gamePlayers;
        for (const auto &it : clients) {
            const auto &clientVal = it.second;
            if (!clientVal.first.isObserver()) {
                gamePlayers.push_back(clientVal.first);
            }
        }
        debug_out_0 << TAG << "Starting new game with " << countReadyPlayers << " ready players and "
                    << clients.size() - countNotObservers << " observers\n";
        std::vector<Record> records = game.startNewGame(gamePlayers, random);
        for (auto &r: records) {
            gameRecords.push_back(r);
        }
        debug_out_0 << TAG << "Game started succesfully " << std::endl;
        queue.setEventsCount(gameRecords.size());
        return records;
    }

    void endGame() {
        debug_out_0 << TAG << " Ending game! " << std::endl;
        for (auto &it : clients) {
            it.second.first.setReady(false);
        }
        // gameRecords.clear();
        countReadyPlayers = 0;
    }

    void performRound() {
        auto records = game.doRound();
        for (auto &r_it : records) {
            gameRecords.push_back(r_it);
        }
        queue.setEventsCount(gameRecords.size());
        //pushPacketsForAll(constructPackets(records));
        if (!game.isGameNow()) {
            endGame();
        }
    }

    MsgQueue& getQueue() {
        return queue;
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


    void sendPacketToClient(const Client &client, const WritePacket &writePacket) const {
        const struct sockaddr_storage *sockaddrStorage = &client.getSockaddrStorage();
        if (sendto(pollServer.getDescriptor(PollServer::MESSAGE_CLIENT), writePacket.getBufferConst(),
                   writePacket.getOffset(), 0,
                   reinterpret_cast<const sockaddr *>(sockaddrStorage),
                   sizeof(client.getSockaddrStorage())) == -1) {
            syserr("error sending udp packet!");
        }
    }

public:
    explicit Server(const ServerData &serverData) : serverData(serverData),
                                                    manager(serverData.getWidth(), serverData.getHeight(),
                                                            serverData.getTuriningSpeed(), serverData.getSeed()),
                                                    clientSocket(serverData.getPortNum()),
                                                    pollServer(clientSocket.getSocket(),
                                                               serverData.getRoundsPerSec()) {}

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
                if (manager.getGame().isGameNow()) {
                    manager.performRound();
                }
            }
            if (pollServer.hasPollinOccurred(PollServer::TIMEOUT_CLIENT)) {
                debug_out_1 << TAG << "Timeout CLIENT tick" << std::endl;
                manager.addTicksAndCleanInactive();
                readTimeout(pollServer.getDescriptor(PollServer::TIMEOUT_CLIENT));
            }
            // TODO włączanie serwera po kolei go psuje :(
            // Konkretnie taka kolejność:
            // 1. klient1
            // 2. serwer
            // 3. klient2
            if (pollServer.hasPollinOccurred(PollServer::MESSAGE_CLIENT)) {
                // debug_out_1 << TAG << "Client message!" << std::endl;
                try {
                    struct sockaddr_storage clientAddr;
                    ReadPacket packet;
                    readFromClients(clientAddr, packet);
                    ClientMessage clientMessage = ClientMessage::decode(packet);
                    // debug_out_1 << clientMessage << std::endl;
                    Client client(clientAddr);
                    manager.handleClient(client, clientMessage);
                    if (!manager.getGame().isGameNow()) {
                        if (manager.canGameStart()) {
                            manager.startGame();
//                            std::cout << "NEW GAME EVENTS " << std::endl;
//                            for (auto &r_it : r) {
//                                std::cout << "EVENT " << r_it.getEventNo() << ", " << r_it.getEventData()->getType()
//                                          << std::endl;
//                            }
//                            std::cout << "pusing packets done!" << std::endl;
                        } else { ;
                            debug_out_1 << "Game cannot start!" << std::endl;
                        }
                    }
                } catch (const Packet::PacketToSmallException &p) {
                    debug_out_0 << "Error decoding client message!" << std::endl;
                    continue;
                }
            }
            if (pollServer.hasPolloutOccurred(PollServer::MESSAGE_CLIENT) && manager.hasMessages()) {
                auto packetToSend = manager.handleNextInQueue();
                sendPacketToClient(packetToSend.second, packetToSend.first);
                pollServer.removePolloutFromEvents(PollServer::MESSAGE_CLIENT);
//                debug_out_1 << "Removing pollout " << std::endl;
            }
            if (manager.hasMessages()) {
//                debug_out_1 << "Adding POLLOUT" << std::endl;
                pollServer.addPolloutToEvents(PollServer::MESSAGE_CLIENT);
            }
        }
    }
};

#endif //ZADANIE_2_SERVER_H
