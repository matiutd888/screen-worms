//
// Created by mateusz on 23.05.2021.
//

#include <ostream>
#include "Connection.h"
#include "DataBuilders.h"
#include <ext/stdio_filebuf.h>
#include <netinet/tcp.h>
#include <chrono>
//Klient:
//
//./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]
//game_server – adres (IPv4 lub IPv6) lub nazwa serwera gry
//-n player_name – nazwa gracza, zgodna z opisanymi niżej wymaganiami
//-p n – port serwera gry (domyślne 2021)
//-i gui_server – adres (IPv4 lub IPv6) lub nazwa serwera obsługującego interfejs użytkownika (domyślnie localhost)
//-r n – port serwera obsługującego interfejs użytkownika (domyślnie 20210)

enum GUIMessage {
    LEFT_KEY_DOWN,
    LEFT_KEY_UP,
    RIGHT_KEY_DOWN,
    RIGHT_KEY_UP,
    NO_MSG,
};

#define LEFT_KEY_DOWN_STR "LEFT_KEY_DOWN"
#define RIGHT_KEY_DOWN_STR "RIGHT_KEY_DOWN"
#define LEFT_KEY_UP_STR "LEFT_KEY_UP"
#define RIGHT_KEY_UP_STR "RIGHT_KEY_UP"

inline GUIMessage getGuiMessage(const std::string &guiMsgStr) {
    if (guiMsgStr == LEFT_KEY_DOWN_STR) {
        return GUIMessage::LEFT_KEY_DOWN;
    }
    if (guiMsgStr == LEFT_KEY_UP_STR) {
        return GUIMessage::LEFT_KEY_UP;
    }
    if (guiMsgStr == RIGHT_KEY_DOWN_STR) {
        return GUIMessage::RIGHT_KEY_DOWN;
    }
    if (guiMsgStr == RIGHT_KEY_UP_STR) {
        return GUIMessage::RIGHT_KEY_UP;
    }
    throw std::invalid_argument("No GUI Message matching!");
}

class ClientSide {
    ClientData data;
    TCPClientSocket guiSock;
    UDPClientSocket sock;
    PollClient pollClient;
    GUIMessage lastGuiMessage;
    TurnDirection direction;
    bool isInGame = false;
    uint32_t currGameId;
    uint32_t nextExpectedEventNo;
    std::vector<std::string> players;
    uint64_t sessionID;

    void handleGuiMessage(const std::string &guiMsgStr) {
        GUIMessage guiMessage;
        try {
            guiMessage = getGuiMessage(guiMsgStr);
            if (guiMessage != lastGuiMessage) {
                debug_out_1 << "CHANGED DIRECTION ";
            }
            if (guiMessage == LEFT_KEY_UP) {
                if (lastGuiMessage == LEFT_KEY_DOWN) {
                    direction = TurnDirection::STRAIGHT;
                    lastGuiMessage = guiMessage;
                }
            }
            if (guiMessage == RIGHT_KEY_UP) {
                if (lastGuiMessage == RIGHT_KEY_DOWN) {
                    direction = TurnDirection::STRAIGHT;
                    lastGuiMessage = guiMessage;
                }
            }
            if (guiMessage == RIGHT_KEY_DOWN) {
                direction = TurnDirection::RIGHT;
                lastGuiMessage = guiMessage;
            }
            if (guiMessage == LEFT_KEY_DOWN) {
                direction = TurnDirection::LEFT;
                lastGuiMessage = guiMessage;
            }
            debug_out_1 << "currDirection = " << direction << std::endl;
        } catch (...) {
            std::cerr << "[GUI] " << " GUI MESSAGE NOT VALID " << std::endl;
        }
    }

    void handleGameOver() {
        debug_out_0 << "RECEIVED GAME OVER: nextExpectedEventNo = " << nextExpectedEventNo << std::endl;
        isInGame = false;
        currGameId = 0;
    }

    void handlePlayerEliminated(Record &record) {
        std::shared_ptr<PlayerEliminatedData> playerEliminatedData = (const std::shared_ptr<PlayerEliminatedData> &) record.getEventData();
        std::string playerStr = "PLAYER_ELIMINATED";
        uint8_t playerNumber = playerEliminatedData->getPlayerNumber();
        if (playerNumber >= players.size()) {
            std::cerr << "Player Eliminated: Ivalid player number! " << std::endl;
            return;
        }
        playerStr.append(" ");
        playerStr.append(players[playerNumber]);
        sendLineToGUI(playerStr);
    }

    void handlePixel(Record &record) {
        std::shared_ptr<PixelEventData> pixelData = (const std::shared_ptr<PixelEventData> &) record.getEventData();
        std::string pixelStr = "PIXEL";
        uint8_t playerNumber = pixelData->getPlayerNumber();
        if (playerNumber >= players.size()) {
            std::cerr << "Pixel eliminated: Invalid player number! " << std::endl;
            return;
        }
        pixelStr.append(" ");
        pixelStr.append(std::to_string(pixelData->getX()));
        pixelStr.append(" ");
        pixelStr.append(std::to_string(pixelData->getY()));

        pixelStr.append(" ");
        pixelStr.append(players[playerNumber]);
        std::cout << pixelStr << std::endl;
        sendLineToGUI(pixelStr);
    }

    void handleGameRecord(Record &record) {
        ServerEventType eventType = record.getEventType();
        uint32_t eventNo = record.getEventNo();
        debug_out_1 << "HANDLE GAME RECORD: " << eventType << " eventNo " << eventNo << std::endl;
        if (eventNo != nextExpectedEventNo)
            return;
        nextExpectedEventNo++;
        if (eventType == ServerEventType::GAME_OVER) {
            handleGameOver();
            return;
        }
        if (eventType == ServerEventType::PLAYER_ELIMINATED) {
            handlePlayerEliminated(record);
        }
        if (eventType == ServerEventType::PIXEL) {
            handlePixel(record);
        }
    }

public:
//
    explicit ClientSide(const ClientData &clientData) : data(clientData),
                                                        guiSock(data.getGuiPortNum(), data.getGuiAddress().c_str()),
                                                        sock(data.getServerPortNum(), data.getServerAddress().c_str()),
                                                        pollClient(sock.getSocket(), guiSock.getSocket()),
                                                        lastGuiMessage(NO_MSG), direction(TurnDirection::STRAIGHT),
                                                        isInGame(false), nextExpectedEventNo(0),
                                                        sessionID(std::chrono::duration_cast<std::chrono::milliseconds>(
                                                                std::chrono::system_clock::now().time_since_epoch()).count()) {
        std::cout << "constructed clientside!\n";
    }

    void sendLineToGUI(const std::string &line) {
        debug_out_0 << "SENDING LINE TO GUI: " << line << std::endl;
        std::string lineWithNewLine = line + "\n";
        send(pollClient.getDescriptor(PollClient::MESSAGE_GUI), lineWithNewLine.c_str(), lineWithNewLine.size(), 0);
    }

    void tryToReadNewGame(ReadPacket &packet, uint32_t rcvdGameId) {
        Record record = Record::decode(packet);
        if (record.getEventType() != ServerEventType::NEW_GAME) {
            debug_out_0 << "Excpected New game but didnt receive it" << std::endl;
            throw EventData::InvalidTypeException();
        }
        debug_out_0 << "Received new Game " << std::endl;
        if (record.getEventNo() != 0) {
            debug_out_1 << "New game event no != 0" << std::endl;
            return;
        }
        debug_out_0 << "New game comunicate parsing " << std::endl;
        std::shared_ptr<NewGameData> newGamePtr = (const std::shared_ptr<NewGameData> &) record.getEventData();
        debug_out_0 << "New game comunicate correct " << std::endl;
        std::cout << *newGamePtr << std::endl;
        std::string newGameString = "NEW_GAME";
        newGameString.append(" ");
        newGameString.append(std::to_string(newGamePtr->getX()));
        newGameString.append(" ");
        newGameString.append(std::to_string(newGamePtr->getY()));
        players = newGamePtr->getPlayerNames();
        for (const auto &player : players) {
            newGameString.append(" ");
            newGameString.append(player);
        }
        sendLineToGUI(newGameString);
        isInGame = true;
        debug_out_1 << "WILL NOW BE READING ONLY " << rcvdGameId << " comunicates!\n";
        currGameId = rcvdGameId;
        nextExpectedEventNo = 1;
    }
	
    void start() {
        std::cout << "starting client " << std::endl;
        __gnu_cxx::stdio_filebuf<char> filebuf(pollClient.getDescriptor(PollClient::MESSAGE_GUI), std::ios::in);
        std::istream guiInputStream(&filebuf);
        while (1) {
            int pollCount = pollClient.doPoll();
            if (pollCount < 0) {
                std::cout << "Error in doPoll!" << std::endl;
                continue;
            }
            if (pollCount == 0) {
                std::cout << "No DoPol events!" << std::endl;
            }
            if (pollClient.hasPollinOccurred(PollClient::MESSAGE_GUI)) {
                debug_out_1 << "MESSAGE GUI " << std::endl;
                std::string guiMsgStr;
                if (getline(guiInputStream, guiMsgStr)) {
                    debug_out_1 << "[GUI] " << "read " << guiMsgStr << " from GUI!" << std::endl;
                    handleGuiMessage(guiMsgStr);
                } else {
                    std::cerr << "Invalid read from GUI " << std::endl;
                    exit(1);
                }
            }
            if (pollClient.hasPollinOccurred(PollClient::INTERVAL_SENDMESSAGE)) {
				uint64_t exp; // Reading timeout.
				ssize_t s = read(pollClient.getDescriptor(PollClient::INTERVAL_SENDMESSAGE), &exp, sizeof(uint64_t));
				if (s != sizeof(uint64_t))
					perror("read timeout pollutils");
	
                // std::cout << "Sending message to server!" << std::endl;
                ClientMessage clientMessage(sessionID, static_cast<uint8_t>(direction), nextExpectedEventNo,
                                            data.getPlayerName().c_str(), data.getPlayerName().size());
                WritePacket writePacket;
                clientMessage.encode(writePacket);
                sendto(sock.getSocket(), writePacket.getBufferConst(), writePacket.getOffset(), 0,
                       sock.getAddrInfo().ai_addr, sock.getAddrInfo().ai_addrlen);
            }
            if (pollClient.hasPollinOccurred(PollClient::MESSAGE_SERVER)) {
                debug_out_1 << "Message server!" << std::endl;
                ReadPacket readPacket;
                sockaddr_storage sockaddrStorage;
                socklen_t addrLen;
                ssize_t size = recvfrom(pollClient.getDescriptor(PollClient::MESSAGE_SERVER), readPacket.getBuffer(),
                                        readPacket.getMaxSize(), 0, reinterpret_cast<sockaddr *>(&sockaddrStorage),
                                        &addrLen);
                readPacket.setSize(size);
                uint32_t rcvdGameID = ServerMessage::getGameId(readPacket);
                debug_out_1 << "received game id = " << rcvdGameID << std::endl;
                if (isInGame && rcvdGameID != currGameId) {
                    debug_out_0 << "received Game id is incorrect!" << std::endl;
                    try {
                        tryToReadNewGame(readPacket, rcvdGameID);
                    } catch (const EventData::InvalidTypeException &e) {
                        std::cerr << "Invalid game ID and didnt receive new game!" << std::endl;
                        continue;
                    }
                }
                try {
                    // Unfortunately, I wasn't able to make code for the case of valid CRC and invalid rest of the msg.
                    if (!isInGame) {
                        std::cout << " is NOT is game!" << std::endl;
                        tryToReadNewGame(readPacket, rcvdGameID);

                    }
                    size_t initialOffset = readPacket.getOffset();
                    while (readPacket.getRemainingSize() > 0 && isInGame) {
                        Record record = Record::decode(readPacket);
                        handleGameRecord(record);
                        if (!record.checkCrcOk(readPacket, initialOffset)) {
                            throw CrcMaker::InvalidCrcException();
                        }
                        initialOffset = readPacket.getOffset();
                    }
                } catch (const Packet::PacketToSmallException &p) {
                    std::cerr << "Some error reading  " << std::endl;
                } catch (const EventData::InvalidTypeException &e) {
                    std::cerr << "Expected New Game but didnt get it!" << std::endl;
                } catch (const Packet::FatalDecodingException &p) {
                    std::cerr << "Decoding exception! " << std::endl;
                } catch (const CrcMaker::InvalidCrcException &e) {
                    std::cerr << "Invalid CRC, skipping package!" << std::endl;
                }
            }

        }
    }
};

int main(int argc, char **argv) {
    ClientDataBuilder builder;
    builder.parse(argc, argv);
    ClientData clientData = builder.build();
    std::cout << clientData << std::endl;
    ClientSide clientSide(clientData);
    clientSide.start();
}
