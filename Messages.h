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

class CrcMaker {
    static uint32_t* crcTable() {
        static uint32_t mTable[256] = {
                0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
                0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
                0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
                0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
                0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
                0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
                0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
                0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
                0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
                0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
                0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
                0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
                0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
                0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
                0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
                0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
                0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
                0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
                0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
                0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
                0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
                0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
                0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
                0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
                0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
                0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
                0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
                0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
                0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
                0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
                0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
                0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
                0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
                0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
                0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
                0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
                0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
                0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
        };
        return mTable;
    }
public:
    class InvalidCrcException : std::exception {
        const char * what() const noexcept override {
            return "Invalid crc32!";
        }
    };

    static uint32_t genCRC(const char *buffer, size_t bufsize) {
        uint32_t crc = 0xffffffff;
        uint32_t index;
        for (size_t i = 0; i < bufsize; i++) {
            index = (crc ^ crcTable()[i]) & 0xFF;
            std::cout << "index = " << index << std::endl;
            crc = (crc >> 8) ^ crcTable()[index];
        }
        return crc ^ 0xffffffff;
    }
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
//        std::cout << "encoding name " << playerName << " of size " << playerNameSize << std::endl;
        packet.write(playerName, playerNameSize);
    };

    static ClientMessage decode(ReadPacket &packet) {
        ClientMessage clientMessage;

        //debug_out_1 << "Decoding Client Message" << std::endl;
        if (packet.getRemainingSize() < minimumMessageSize()) {
            throw Packet::PacketToSmallException();
        }
        read64FromPacket(packet, clientMessage.sessionID);
        //debug_out_1 << "Read session ID = " << clientMessage.sessionID << std::endl;
        packet.readData(&clientMessage.turnDirection, sizeof(turnDirection));
        //debug_out_1 << "READ turn direction = " << uint64_t(clientMessage.turnDirection) << std::endl;
        if (clientMessage.turnDirection > Utils::TURN_MAX_VALUE)
            throw Packet::FatalDecodingException();

        read32FromPacket(packet, clientMessage.nextExpectedEventNo);
        //debug_out_1 << " READ next expected event no = " << clientMessage.nextExpectedEventNo << std::endl;

        size_t remainingSize = packet.getRemainingSize();
        if (remainingSize > MAX_PLAYER_NAME_LENGTH) {
            std::cerr << "ERROR: Remaining size  > MAX_PLAYER_NAME_LENGTH " << std::endl;
            throw Packet::FatalDecodingException();
        }
        if (remainingSize == 0) {
            clientMessage.playerNameSize = 0;
            return clientMessage;
        }
//        std::cout << "REMAINING SIZE = " << remainingSize << std::endl;
        packet.readData(clientMessage.playerName, remainingSize);
        std::string s;
//        std::cout << clientMessage.playerName << " " << std::endl;
        clientMessage.playerNameSize = remainingSize;
        s.resize(clientMessage.getPlayerNameSize());
        memcpy(s.data(), clientMessage.playerName, clientMessage.playerNameSize);
//        debug_out_1 << "READ player name string = " << s << std::endl;
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
    [[nodiscard]] std::string getStringPlayerName() const {
        std::string s;
        s.resize(playerNameSize);
        memcpy(s.data(), playerName, playerNameSize);
        return s;
    };
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

    class InvalidTypeException : std::exception {
        const char * what() const noexcept override {
            return "Expected diferent type of event! (type invalid or just not expected)\n";
        }
    };
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

    static std::shared_ptr<EventData> decode(ReadPacket &packet, uint32_t eventDataLen) {
        NewGameData newGameData;
        read32FromPacket(packet, newGameData.x);
        read32FromPacket(packet, newGameData.y);
        std::string playersBuff;
        size_t remainingSize = eventDataLen - sizeof(newGameData.x) - sizeof(newGameData.y);
        playersBuff.resize(remainingSize);
        packet.readData(playersBuff.data(), remainingSize);
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

    [[nodiscard]] uint32_t getX() const {
        return x;
    }

    [[nodiscard]] uint32_t getY() const {
        return y;
    }

    [[nodiscard]] const std::vector<std::string> &getPlayerNames() const {
        return playerNames;
    }

    [[nodiscard]] size_t getPlayerNameSize() const {
        return playerNameSize;
    }

    friend std::ostream &operator<<(std::ostream &os, const NewGameData &data) {
        std::string s;
        for (const auto &piece : data.playerNames) s += piece + " ";
        os << " x: " << data.x << " y: " << data.y << " playerNames: "
           << s << " playerNameSize: " << data.playerNameSize;
        return os;
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

    uint8_t getPlayerNumber() const {
        return playerNumber;
    }

    uint32_t getX() const {
        return x;
    }

    uint32_t getY() const {
        return y;
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

    uint8_t getPlayerNumber() const {
        return playerNumber;
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
        std::cout << "Encoding Record: " << std::endl;
        if (getSize() > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        uint32_t sizeNoCRC32 = getSize() - sizeof(crc32_t);
        size_t initialOffset = packet.getOffset();
        std::cout << "Initial offset " << initialOffset << std::endl;
        std::cout << "Len = " << len << std::endl;
        write32ToPacket(packet, len);
        std::cout << "eventNo = " << eventNo << std::endl;
        write32ToPacket(packet, eventNo);
        std::cout << "eventDataType = " << eventData->getType() << std::endl;
        eventData->encode(packet);
        uint32_t m_crc32 = CrcMaker::genCRC(packet.getBufferWithOffsetConst(initialOffset), sizeNoCRC32);
//        uint32_t m_crc32 = 0;
        write32ToPacket(packet, m_crc32);
    }

    static Record decode(ReadPacket &packet) {
        std::cout << "DECODING PACKET, size = " << packet.getRemainingSize() << std::endl;
        // We want to read len first.
        if (sizeof(uint32_t) > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        Record record;
        read32FromPacket(packet, record.len);
        std::cout << "Received len = " << record.len << std::endl;
        if (record.len + sizeof(crc32_t) > packet.getRemainingSize())
            throw Packet::PacketToSmallException();
        read32FromPacket(packet, record.eventNo);
        std::cout << "Received eventNo = " << record.eventNo << std::endl;
        uint8_t event_type;
        packet.readData(&event_type, sizeof(uint8_t));
        std::cout << "Received eventType = " << event_type << std::endl;
        if (event_type > max_event_type)
            throw Packet::FatalDecodingException();
        auto eventType = static_cast<ServerEventType>(event_type);
        std::shared_ptr<EventData> ev;
        switch (eventType) {
            case ServerEventType::PIXEL:
                ev = PixelEventData::decode(packet);
                break;
            case ServerEventType::NEW_GAME:
                ev = NewGameData::decode(packet, record.len - sizeof(uint8_t) - sizeof(uint32_t))     ;
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

    [[nodiscard]] const std::shared_ptr<EventData> &getEventData() const {
        return eventData;
    }

    [[nodiscard]] uint32_t getEventNo() const {
        return eventNo;
    }

    [[nodiscard]] ServerEventType getEventType() const {
        return eventData->getType();
    }

    bool checkCrcOk(ReadPacket &packet, size_t initialOffset) {
        return crc32 == CrcMaker::genCRC(packet.getBufferWithOffsetConst(initialOffset), getSize() - sizeof(crc32_t));
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
