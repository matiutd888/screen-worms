//
// Created by mateusz on 21.05.2021.
//

#ifndef ZADANIE_2_MESSAGES_H
#define ZADANIE_2_MESSAGES_H


#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <netinet/in.h>
#include "Packet.h"
#include "err.h"


inline bool writeShortToPacket(Packet &packet, uint16_t x) {
    uint16_t networkx = htons(x);
    return packet.write(&networkx, sizeof(networkx));
}

inline bool writeLongToPacket(Packet &packet, uint32_t x) {
    uint32_t networkx = htonl(x);
    return packet.write(&networkx, sizeof(networkx));
}

class Message {
    static uint32_t crcTable[256];
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
            crcTable[(size_t) b] = remainder;
        } while (0 != ++b);
    }

    static uint32_t genCRC(char *buffer, size_t bufsize) {
        uint32_t crc = 0xffffffff;
        size_t i;
        for (i = 0; i < bufsize; i++)
            crc = crcTable[*buffer++ ^ (crc & 0xff)] ^ (crc >> 8);
        return (~crc);
    }

    virtual bool encode(Packet &packet) = 0;

    virtual void decode() = 0;
};

class ClientMessage : Message {
    uint64_t sessionID;
    uint8_t turnDirection;
    uint32_t nextExpectedEventNo;
    std::string player_name;
public:
//    virtual bool encode(Packet &packet) override {};
//
//    virtual void decode() override {};
};

enum ServerEventType {
    NEW_GAME = 0, PIXEL = 1, PLAYER_ELIMINATED = 2, GAME_OVER = 3
};

class EventData {
protected:
    ServerEventType type;
public:
    EventData() = default;

    virtual bool encode(Packet &packet) = 0;

    virtual void decode() = 0;

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

    bool encode(Packet &packet) override {
        if (getSize() > packet.getRemainingSize())
            return false;
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));

        writeLongToPacket(packet, x);
        writeLongToPacket(packet, y);
        for (const auto &it : playerNames) {
            packet.write(it.c_str(), it.size() + 1);
        }

        return true;
    }

    void decode() override {}
};

class PixelEventData : public EventData {
    uint8_t playerNumber;
    uint32_t x, y;
public:
    PixelEventData(uint8_t player_name, uint32_t x, uint32_t y) : playerNumber(player_name), x(x), y(y) {
        type = ServerEventType::PIXEL;
    }

    size_t getSize() const override {
        return sizeof(playerNumber) + sizeof(x) + sizeof(y) + EventData::getSize();
    }

    bool encode(Packet &packet) override {
        if (getSize() > packet.getRemainingSize())
            return false;
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));

        packet.write(&playerNumber, sizeof(playerNumber));
        writeLongToPacket(packet, x);
        writeLongToPacket(packet, y);
        return true;
    }

    void decode() override {}
};

class PlayerEliminatedData : public EventData {
    uint8_t playerNumber;
public:
    PlayerEliminatedData(uint8_t player_number) : playerNumber(player_number) {
        type = ServerEventType::PLAYER_ELIMINATED;
    }

    bool encode(Packet &packet) override {
        if (getSize() > packet.getRemainingSize())
            return false;
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));
        packet.write(&playerNumber, sizeof(playerNumber));
        return true;
    }

    size_t getSize() const override {
        return sizeof(playerNumber) + EventData::getSize();
    }

    void decode() override {}
};

class GameOver : public EventData {
public:
    GameOver() {
        type = ServerEventType::GAME_OVER;
    }

    bool encode(Packet &packet) override {
        if (getSize() > packet.getRemainingSize())
            return false;
        uint8_t typeNetwork = type;
        packet.write(&typeNetwork, sizeof(typeNetwork));
        return true;
    }

    void decode() override {

    }
};

class Record : Message {
    uint32_t eventNo;
    uint32_t crc32;
    uint32_t len;
    std::shared_ptr<EventData> eventData;
public:
    Record(uint32_t eventNo, std::shared_ptr<EventData> eventData) {
        this->eventData = std::move(eventData);
        len = this->eventData->getSize() + sizeof(eventNo);
        this->eventNo = eventNo;
    }

    size_t getSize() {
        return len + sizeof(len) + sizeof(crc32);
    }


    bool encode(Packet &packet) {
        if (getSize() < packet.getRemainingSize())
            return false;

        uint32_t sizeNoCRC32 = getSize() - sizeof(crc32);
        size_t initialOffset = packet.getOffset();

        if (!writeLongToPacket(packet, len))
            syserr("Unexpected writing error!\n");

        if (!writeLongToPacket(packet, eventNo))
            syserr("Unexpected writing error!\n");

        if (!eventData->encode(packet))
            syserr("Unexpected writing error!\n");

        crc32 = genCRC(packet.getBufferWithOffset(initialOffset), sizeNoCRC32);
        if (!writeLongToPacket(packet, crc32))
            syserr("Unexpected writing error!\n");

        return true;
    }

    void decode() {

    }
};

class ServerMessage {
    uint32_t game_id;
    std::vector<Record> events;
};

#endif //ZADANIE_2_MESSAGES_H
