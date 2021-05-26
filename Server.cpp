//
// Created by mateusz on 23.05.2021.
//

#include "Server.h"

bool MsgQueue::addClient(Client client, uint32_t nextExpectedEventNo) {
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

bool MsgQueue::hasMessages() const {
    size_t it = nextClientIndex;
    do {
        if (expectedEventNo[it] != NO_EVENT && expectedEventNo[it] < eventsCount)
            return true;
        advanceModulo(it);
    } while (it != nextClientIndex);
    return false;
}

size_t MsgQueue::getNextExpectedClientIndexAndAdvance() {
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

const Client &MsgQueue::getClientByIndex(size_t index) {
    if (index >= CLIENTS_SIZE)
        throw std::invalid_argument("Invalid client index!");
    if (expectedEventNo[index] == NO_EVENT)
        throw std::invalid_argument("No client at this index!");
    return clients[index];
}

void MsgQueue::setExpectedEventsToZero() {
    for (int i = 0; i < CLIENTS_SIZE; ++i) {
        if (expectedEventNo[i] != NO_EVENT) {
            expectedEventNo[i] = 0;
        }
    }
}

void MsgQueue::updateClient(const Client &client, uint32_t nextExpectedEventNo) {
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

void MsgQueue::deleteClientFromQueue(const Client &client) {
    for(size_t it = 0; it < CLIENTS_SIZE; it++) {
        if (clients[it] == client) {
            clients[it] = Client();
            expectedEventNo[it] = NO_EVENT;
        }
    }
}

void MsgQueue::setExpectedEventNo(size_t index, uint32_t value) {
    if (value > eventsCount) {
        throw InvalidEventNoException();
    }
    if (index >= CLIENTS_SIZE)
        throw std::invalid_argument("Invalid client index!");

    expectedEventNo[index] = value;
}

std::pair<WritePacket, Client> ClientManager::handleNextInQueue() {
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

void ClientManager::addClient(const Client &client, const ClientMessage &newMsg) {
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

void ClientManager::removeClient(const std::map<Client, ClientManager::clientValue_t>::iterator &iterator) {
    debug_out_1 << "REMOVING CLIENT " << std::endl;
    const Client &client = iterator->first;
    const Player &player = std::get<PLAYER_INDEX>(iterator->second);
    if (!player.isObserver())
        countNotObservers--;
    if (!game.isGameNow()) {
        if (player.isPlayerReady())
            countReadyPlayers--;
    }
    debug_out_1  << "BEFORE: Clients.count = " << clients.size() << " not observers =  " << countNotObservers
                 << " not ready players = " << countReadyPlayers << std::endl;
    queue.deleteClientFromQueue(iterator->first);
    clients.erase(iterator);
}

void ClientManager::updateClientPlayersInfo(const Client &client, ClientManager::clientValue_t &val,
                                            const ClientMessage &message) {
    Player &player = std::get<PLAYER_INDEX>(val);
    bool before = player.isPlayerReady();
    player.setDirection(static_cast<TurnDirection>(message.getTurnDirection()));
    if (game.isGameNow()) {
        game.updatePlayer(player);
    } else {
        bool after = player.isPlayerReady();
        if (!before && after) {
            countReadyPlayers++;
            debug_out_1 << "PLAYER BECAME READY: " << player << " COUNT READY PLAYERS = " << countReadyPlayers << std::endl;
        }
    }
    std::get<TICKS_INDEX>(val) = 0;
    debug_out_1 << "Updating client: " << client << " queue index!" << std::endl;
    queue.updateClient(client, message.getNextExpectedEventNo());
}

void ClientManager::handleClient(Client &newClient, const ClientMessage &newMsg) {
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

auto ClientManager::startGame() {
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

void ClientManager::endGame() {
    debug_out_0 << TAG << " Ending game! " << std::endl;
    for (auto &it : clients) {
        it.second.first.setReady(false);
    }
    countReadyPlayers = 0;
}

void ClientManager::performRound() {
    auto records = game.doRound();
    for (auto &r_it : records) {
        gameRecords.push_back(r_it);
    }
    queue.setEventsCount(gameRecords.size());
    if (!game.isGameNow()) {
        endGame();
    }
}

void Server::readTimeout(int fd) {
    uint64_t exp;
    ssize_t s = read(fd, &exp, sizeof(uint64_t));
    if (s != sizeof(uint64_t))
        perror("read timeout pollutils");
}

void Server::readFromClients(sockaddr_storage &clientAddr, ReadPacket &packet) {
    std::cout << "READ FROM CLIENTS!" << std::endl;
    socklen_t clientAddrLen = sizeof(clientAddr);

    ssize_t numBytes = recvfrom(pollServer.getDescriptor(PollServer::MESSAGE_CLIENT),
                                packet.getBuffer(),
                                packet.getMaxSize(),
                                0,
                                reinterpret_cast<sockaddr *>(&clientAddr),
                                &clientAddrLen);


    if (numBytes < 0) {
        debug_out_1 << TAG << "ERROR READING FROM CLIENT " << std::endl;
        throw Packet::FatalDecodingException();
    }
    packet.setSize(numBytes);
}

void Server::sendPacketToClient(const Client &client, const WritePacket &writePacket) const {
    const struct sockaddr_storage *sockaddrStorage = &client.getSockaddrStorage();
    if (sendto(pollServer.getDescriptor(PollServer::MESSAGE_CLIENT), writePacket.getBufferConst(),
               writePacket.getOffset(), 0,
               reinterpret_cast<const sockaddr *>(sockaddrStorage),
               sizeof(client.getSockaddrStorage())) == -1) {
        syserr("error sending udp packet!");
    }
}

void Server::start() {
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
        if (pollServer.hasPollinOccurred(PollServer::MESSAGE_CLIENT)) {
            try {
                struct sockaddr_storage clientAddr;
                ReadPacket packet;
                readFromClients(clientAddr, packet);
                ClientMessage clientMessage = ClientMessage::decode(packet);
                Client client(clientAddr);
                manager.handleClient(client, clientMessage);
                if (!manager.getGame().isGameNow()) {
                    if (manager.canGameStart()) {
                        manager.startGame();
                    } else {
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
        }
        if (manager.hasMessages()) {
            pollServer.addPolloutToEvents(PollServer::MESSAGE_CLIENT);
        }
    }
}
