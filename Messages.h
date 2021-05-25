//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_MESSAGES_H
#define ZADANIE_2_MESSAGES_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <memory>
#include <netinet/in.h>
#include "Packet.h"
#include "err.h"
#include <iostream>
#include "Utils.h"

inline void read32FromPacket(ReadPacket &packet, uint32_t &x) {
    uint32_t networkx;
    packet.readData(&networkx, sizeof(networkx));
    x = ntohl(networkx);
}

inline void read64FromPacket(ReadPacket &packet, uint64_t &x) {
    uint64_t networkx;
    packet.readData(&networkx, sizeof(networkx));
    x = be64toh(networkx);
//    debug_out << "networkx " << networkx << std::endl;
//    debug_out << "x " << x << std::endl;
}

inline void writeShortToPacket(WritePacket &packet, uint16_t x) {
    uint16_t networkx = htons(x);
    packet.write(&networkx, sizeof(networkx));
}

inline void write32ToPacket(WritePacket &packet, uint32_t x) {
    uint32_t networkx = htonl(x);
    packet.write(&networkx, sizeof(networkx));
}

inline void write64ToPacket(WritePacket &packet, uint64_t x) {
    uint64_t networkx = htobe64(x);
    packet.write(&networkx, sizeof(networkx));
}
inline bool charOK(char c) {
    if (c < 33 || c > 126)
        return false;
    return true;
}

class Message {
    static uint32_t* crcTable() {
        static uint32_t mTable[256];
        return mTable;
    }
public:
    // https://stackoverflow.com/questions/26049150/calculate-a-32-bit-crc-lookup-table-in-c-c
    static void makeCRCtable() {
        unsigned long POLYNOMIAL = 0xEDB88320;
        unsigned long remainder;
        unsigned char b = 0;
        do {
            // Start with the data byte
            remainder = b;
            for (unsigned long bit = 8; bit > 0; --bit) {
                if (remainder & 1)
                    remainder = (remainder >> 1) ^ POLYNOMIAL;
                else
                    remainder = (remainder >> 1);
            }
            crcTable()[(size_t) b] = remainder;
        } while (0 != ++b);
    }

    uint32_t genCRC(const char *buffer, size_t bufsize) {
        uint32_t crc = 0xffffffff;
        size_t i;
        for (i = 0; i < bufsize; i++)
            crc = crcTable()[*buffer++ ^ (crc & 0xff)] ^ (crc >> 8);
        return (~crc);
    }

//    virtual void encode(WritePacket &packet) = 0;
//
//    virtual bool decode(const ReadPacket &packet) = 0;
};

class ClientMessage {
    uint64_t sessionID;
    uint8_t turnDirection;
    uint32_t nextExpectedEventNo;
    static constexpr size_t MAX_PLAYER_NAME_LENGTH = 20;
    char playerName[MAX_PLAYER_NAME_LENGTH];
    size_t playerNameSize;
    ClientMessage() = default;
    static bool checkIfNameOK(const char *name, size_t nameSize) {
        for (size_t i = 0; i < nameSize; i++) {
            if (!charOK(name[i]))
                return false;
        }
        return true;
    }
public:
    ClientMessage(uint64_t sessionId, uint8_t turnDirection, uint32_t nextExpectedEventNo, const char *playerName, size_t playerNameSize)
            : sessionID(sessionId), turnDirection(turnDirection), nextExpectedEventNo(nextExpectedEventNo), playerNameSize(playerNameSize) {
        if (playerNameSize > MAX_PLAYER_NAME_LENGTH) {
            syserr("Invalid player Name!\n");
        }
        if (!checkIfNameOK(playerName, playerNameSize))
            syserr("Name not legit!\n");
        memcpy(this->playerName, playerName, playerNameSize);
    }

    static size_t minimumMessageSize() {
        return sizeof(uint64_t) + sizeof(uint8_t) + sizeof(uint32_t);
    }

    void encode(WritePacket &packet) {
        write64ToPacket(packet, sessionID);
        packet.write(&turnDirection, sizeof(turnDirection));
        write32ToPacket(packet, nextExpectedEventNo);
        packet.write(playerName, playerNameSize);
    };

    static ClientMessage decode(ReadPacket &packet) {
        ClientMessage clientMessage;

        debug_out_1 << "Decoding Client Message" << std::endl;
        if (packet.getRemainingSize() < minimumMessageSize()) {
            throw Packet::PacketToSmallException();
        }
        read64FromPacket(packet, clientMessage.sessionID);
        debug_out_1 << "Read session ID = " << clientMessage.sessionID << std::endl;
        packet.readData(&clientMessage.turnDirection, sizeof(turnDirection));
        debug_out_1 << "READ turn direction = " << uint64_t(clientMessage.turnDirection) << std::endl;
        if (clientMessage.turnDirection > Utils::TURN_MAX_VALUE)
            throw Packet::FatalDecodingException();

        read32FromPacket(packet, clientMessage.nextExpectedEventNo);
        debug_out_1 << " READ next expected event no = " << clientMessage.nextExpectedEventNo << std::endl;

        size_t remainingSize = packet.getRemainingSize();
        if (remainingSize > MAX_PLAYER_NAME_LENGTH) {
            std::cerr << "ERROR: Remaining size  > MAX_PLAYER_NAME_LENGTH " << std::endl;
            throw Packet::FatalDecodingException();
        }
        packet.readData(clientMessage.playerName, remainingSize);
        debug_out_1 << "READ player name = " << std::string(clientMessage.playerName) << std::endl;
        clientMessage.playerNameSize = remainingSize;
        if (!checkIfNameOK(clientMessage.playerName, clientMessage.playerNameSize))
            throw Packet::FatalDecodingException();
        return clientMessage;
    };

    friend std::ostream &operator<<(std::ostream &os, const ClientMessage &message) {
        os << "sessionID: " << message.sessionID << " turnDirection: " << uint64_t(message.turnDirection)
           << " nextExpectedEventNo: " << message.nextExpectedEventNo << " playerName: " << std::string(message.playerName)
           << " playerNameSize: " << message.playerNameSize;
        return os;
    }

    [[nodiscard]] const char *getPlayerName() const {
        return playerName;
    }

    [[nodiscard]] uint32_t getNextExpectedEventNo() const {
        return nextExpectedEventNo;
    }

    size_t getPlayerNameSize() const {
        return playerNameSize;
    }

    uint64_t getSessionId() const {
        return sessionID;
    }

    uint8_t getTurnDirection() const {
        return turnDirection;
    }
};

enum ServerEventType {
    NEW_GAME = 0, PIXEL = 1, PLAYER_ELIMINATED = 2, GAME_OVER = 3
};

class EventData {
protected:
    ServerEventType type;
public:
    EventData() = default;

    virtual void encode(WritePacket &packet) const = 0;

    [[nodiscard]] virtual size_t getSize() const {
        return 1; // Typ has only one byte.
    };

    virtual ~EventData() = default;

    [[nodiscard]] ServerEventType getType() const {
        return type;
    }
};

class NewGameData : public EventData {
    uint32_t x, y;
    std::vector<std::string> playerNames;
    size_t playerNameSize;
    NewGameData() {type = ServerEventType::NEW_GAME;}
public:
    NewGameData(uint32_t x, uint32_t y, const std::vector<std::string> &playerNames) : x(x), y(y),
                                                                                       playerNames(playerNames) {
        type = ServerEventType::NEW_GAME;
        playerNameSize = 0;
        for (const auto &it : playerNames) {
            playerNameSize += it.size() + 1; // We are counting '\0' char.
        }
    }

    size_t getSize() const override {
        return sizeof(x) + sizeof(y) + playerNameSize + EventData::getSize();
    }

    void encode(WritePacket &packet) const override {
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));

        write32ToPacket(packet, x);
        write32ToPacket(packet, y);
        for (const auto &it : playerNames) {
            packet.write(it.c_str(), it.size() + 1);
        }
    }

    static std::shared_ptr<EventData> decode(ReadPacket &packet) {
        NewGameData newGameData;
        read32FromPacket(packet, newGameData.x);
        read32FromPacket(packet, newGameData.y);
        std::string playersBuff;
        playersBuff.resize(packet.getRemainingSize());
        packet.readData(playersBuff.data(), packet.getRemainingSize());
        // TODO sprawdzić czy to działa
        std::vector<std::string> playerNames;
        std::string stringIt;
        for (int i = 0; i < playersBuff.size(); ++i) {
            if (playersBuff[i] == '\0') {
                playerNames.push_back(stringIt);
                stringIt = "";
            } else {
                if (!charOK(playersBuff[i]))
                    throw Packet::FatalDecodingException();
                stringIt += playersBuff[i];
            }
        }
        if (!stringIt.empty())
            throw Packet::FatalDecodingException();
        newGameData.playerNames = playerNames;
        size_t namesSize = 0;
        for (const auto &it : playerNames) {
            namesSize += it.size() + 1;
        }
        newGameData.playerNameSize = namesSize;
        return std::make_shared<NewGameData>(newGameData);
    }
};

class PixelEventData : public EventData {
    uint8_t playerNumber;
    uint32_t x, y;
    PixelEventData() {
        type = ServerEventType::PIXEL;
    }
public:
    PixelEventData(uint8_t player_name, uint32_t x, uint32_t y) : playerNumber(player_name), x(x), y(y) {
        type = ServerEventType::PIXEL;
    }

    size_t getSize() const override {
        return sizeof(playerNumber) + sizeof(x) + sizeof(y) + EventData::getSize();
    }

    void encode(WritePacket &packet) const override {
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));

        packet.write(&playerNumber, sizeof(playerNumber));
        write32ToPacket(packet, x);
        write32ToPacket(packet, y);
    }
    // Type has already been read.
    static std::shared_ptr<EventData> decode(ReadPacket &packet) {
        PixelEventData pixelEventData;
        packet.readData(&(pixelEventData.playerNumber), sizeof(pixelEventData.playerNumber));
        read32FromPacket(packet, pixelEventData.x);
        read32FromPacket(packet, pixelEventData.y);
        return std::make_shared<PixelEventData>(pixelEventData);
    }
};

class PlayerEliminatedData : public EventData {
    uint8_t playerNumber;
    PlayerEliminatedData() { type = PLAYER_ELIMINATED;};
public:
    PlayerEliminatedData(uint8_t player_number) : playerNumber(player_number) {
        type = ServerEventType::PLAYER_ELIMINATED;
    }

    void encode(WritePacket &packet) const override {
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));
        packet.write(&playerNumber, sizeof(playerNumber));
    }

    [[nodiscard]] size_t getSize() const override {
        return sizeof(playerNumber) + EventData::getSize();
    }

    static std::shared_ptr<EventData> decode(ReadPacket &packet) {
        PlayerEliminatedData playerEliminatedData;
        packet.readData(&playerEliminatedData.playerNumber, sizeof(playerEliminatedData.playerNumber));
        return std::make_shared<PlayerEliminatedData>(playerEliminatedData);
    }
};

class GameOver : public EventData {
public:
    GameOver() {
        type = ServerEventType::GAME_OVER;
    }

    void encode(WritePacket &packet) const override {
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));
    }

    static std::shared_ptr<EventData> decode(ReadPacket &packet) {
        return std::make_shared<GameOver>(GameOver());
    }
};

class Record {
    uint32_t eventNo;
    using crc32_t = uint32_t;
    uint32_t len;
    std::shared_ptr<EventData> eventData;
    uint32_t crc32;
    Record() = default;
    static uint8_t constexpr max_event_type = 3;
public:
    Record(uint32_t eventNo, std::shared_ptr<EventData> eventData) {
        this->eventData = std::move(eventData);
        len = this->eventData->getSize() + sizeof(eventNo);
        this->eventNo = eventNo;
    }

    size_t getSize() const {
        return len + sizeof(len) + sizeof(crc32_t); // Len already has size of eventData.
    }

    void encode(WritePacket &packet) const {
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint32_t sizeNoCRC32 = getSize() - sizeof(crc32_t);
        size_t initialOffset = packet.getOffset();
        write32ToPacket(packet, len);
        write32ToPacket(packet, eventNo);
        eventData->encode(packet);
        Message message;
        uint32_t m_crc32 = message.genCRC(packet.getBufferWithOffsetConst(initialOffset), sizeNoCRC32);
        write32ToPacket(packet, m_crc32);
    }

    static Record decode(ReadPacket &packet) {
        // We want to read len first.
        if (sizeof(uint32_t) > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        Record record;
        read32FromPacket(packet, record.len);
        if (record.len + sizeof(crc32_t) < packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        read32FromPacket(packet, record.eventNo);
        uint8_t event_type;
        packet.readData(&event_type, sizeof(uint8_t));
        if (event_type > max_event_type)
            throw Packet::FatalDecodingException();
        ServerEventType eventType = static_cast<ServerEventType>(event_type);
        std::shared_ptr<EventData> ev;
        switch (eventType) {
            case ServerEventType::PIXEL:
                ev = PixelEventData::decode(packet);
                break;
            case ServerEventType::NEW_GAME:
                ev = NewGameData::decode(packet);
                break;
            case ServerEventType::PLAYER_ELIMINATED:
                ev = PlayerEliminatedData::decode(packet);
            case ServerEventType::GAME_OVER:
                ev = GameOver::decode(packet);
        }
        record.eventData = ev;
        read32FromPacket(packet, record.crc32);
        return record;
    }
};

class ServerMessage {
public:
    static void startServerMessage(WritePacket &packet, uint32_t gameId) {
        if (packet.getRemainingSize() < sizeof(gameId))
            throw Packet::PacketToSmallException();
        write32ToPacket(packet, gameId);
    }

    // gets gameID
    static uint32_t getGameId(ReadPacket &packet) {
        uint32_t gameId;
        read32FromPacket(packet, gameId);
        return gameId;
    }
};

#endif //ZADANIE_2_MESSAGES_H