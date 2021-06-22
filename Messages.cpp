//
// Created by mateusz on 21.05.2021.
//

#include "Messages.h"


#include <sys/socket.h>
#include <netinet/in.h>

std::shared_ptr<EventData> NewGameData::decode(ReadPacket &packet, uint32_t eventDataLen) {
    NewGameData newGameData;
    read32FromPacket(packet, newGameData.x);
    read32FromPacket(packet, newGameData.y);
    std::string playersBuff;
    size_t remainingSize = eventDataLen - sizeof(newGameData.x) - sizeof(newGameData.y);
    playersBuff.resize(remainingSize);
    packet.readData((char *)playersBuff.data(), remainingSize);
    std::vector<std::string> playerNames;
    std::string stringIt;
    for (size_t i = 0; i < playersBuff.size(); ++i) {
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

void PixelEventData::encode(WritePacket &packet) const {
    if (getSize() > packet.getRemainingSize())
        throw Packet::PacketToSmallException();
    uint8_t typeNetwork = type;
    packet.write(&typeNetwork, sizeof(typeNetwork));

    packet.write(&playerNumber, sizeof(playerNumber));
    write32ToPacket(packet, x);
    write32ToPacket(packet, y);
}

std::shared_ptr<EventData> PixelEventData::decode(ReadPacket &packet) {
    PixelEventData pixelEventData;
    packet.readData(&(pixelEventData.playerNumber), sizeof(pixelEventData.playerNumber));
    read32FromPacket(packet, pixelEventData.x);
    read32FromPacket(packet, pixelEventData.y);
    return std::make_shared<PixelEventData>(pixelEventData);
}

void PlayerEliminatedData::encode(WritePacket &packet) const {
    if (getSize() > packet.getRemainingSize())
        throw Packet::PacketToSmallException();
    uint8_t typeNetwork = type;
    packet.write(&typeNetwork, sizeof(typeNetwork));
    packet.write(&playerNumber, sizeof(playerNumber));
}

std::shared_ptr<EventData> PlayerEliminatedData::decode(ReadPacket &packet) {
    PlayerEliminatedData playerEliminatedData;
    packet.readData(&playerEliminatedData.playerNumber, sizeof(playerEliminatedData.playerNumber));
    return std::make_shared<PlayerEliminatedData>(playerEliminatedData);
}

void ClientMessage::encode(WritePacket &packet) {
    write64ToPacket(packet, sessionID);
    packet.write(&turnDirection, sizeof(turnDirection));
    write32ToPacket(packet, nextExpectedEventNo);
    packet.write(playerName, playerNameSize);
}
