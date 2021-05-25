//
// Created by mateusz on 23.05.2021.
//

#include <ostream>
#include "Connection.h"
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

class ClientData {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
public:
    ClientData(const std::string &serverAddress, const std::string &playerName, uint16_t serverPortNum,
               const std::string &guiAddress, uint16_t guiPortNum) : serverAddress(serverAddress),
                                                                     playerName(playerName),
                                                                     serverPortNum(serverPortNum),
                                                                     guiAddress(guiAddress), guiPortNum(guiPortNum) {}


    friend std::ostream &operator<<(std::ostream &os, const ClientData &data) {
        os << "serverAddress: " << data.serverAddress << " playerName: " << data.playerName << " serverPortNum: "
           << data.serverPortNum << " guiAddress: " << data.guiAddress << " guiPortNum: " << data.guiPortNum;
        return os;
    }

    [[nodiscard]] const std::string &getServerAddress() const {
        return serverAddress;
    }

    [[nodiscard]] const std::string &getPlayerName() const {
        return playerName;
    }

    [[nodiscard]] uint16_t getServerPortNum() const {
        return serverPortNum;
    }

    [[nodiscard]] const std::string &getGuiAddress() const {
        return guiAddress;
    }

    [[nodiscard]] uint16_t getGuiPortNum() const {
        return guiPortNum;
    }
};

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

class ClientDataBuilder {
    std::string serverAddress;
    std::string playerName;
    uint16_t serverPortNum;
    std::string guiAddress;
    uint16_t guiPortNum;
    uint32_t next_expected_event_no;
public:
    ClientDataBuilder() {
        serverAddress = "";
        playerName = "";
        serverPortNum = Utils::DEFAULT_SERVER_PORT_NUM;
        guiAddress = Utils::DEFAULT_GUI_SERVER_NAME;
        guiPortNum = Utils::DEFAULT_GUI_PORT_NUM;
    }

    void parse(int argc, char **argv) {
        char c;
        if (argc < 2)
            syserr("Client usage: game_server [-n player_name] [-p n] [-i gui_server] [-r n]");
        serverAddress = argv[1];
        optind = 2;
        while ((c = getopt(argc, argv, "n:p:i:r:")) != -1) {
            std::string argStr(optarg);
            size_t convertedSize = 0;
            printf("%c\n", c);
            bool haveFoundNumber = false;
            switch (c) {
                case 'n':
                    playerName = argStr;
                    break;
                case 'p':
                    serverPortNum = std::stoi(argStr    , &convertedSize);
                    haveFoundNumber = true;
                    break;
                case 'i':
                    guiAddress = argStr;
                    break;
                case 'r':
                    guiPortNum = std::stoi(argStr, &convertedSize);
                    haveFoundNumber = true;
                    break;
                default:;
            }
            if (haveFoundNumber) {
                if (convertedSize != argStr.size())
                    syserr("Argument parsing error!");
            }
        }
    }

    ClientData build() const {
        return ClientData(serverAddress, playerName, serverPortNum, guiAddress, guiPortNum);
    }
};

class ClientSide {
    ClientData data;
    TCPClientSocket guiSock;
    UDPClientSocket sock;
    PollClient pollClient;
    TurnDirection direction;
    GUIMessage lastGuiMessage;
    bool isInGame = false;
    uint32_t currGameId;
    uint32_t nextExpectedEventNo = 0;
    std::vector<std::string> players;
    uint64_t sessionID;
    void handleGuiMessage(const std::string &guiMsgStr) {
        GUIMessage guiMessage;
        try {
            guiMessage = getGuiMessage(guiMsgStr);
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
                if (lastGuiMessage != LEFT_KEY_DOWN) {
                    direction = TurnDirection::RIGHT;
                    lastGuiMessage = guiMessage;
                }
            }
            if (guiMessage == LEFT_KEY_DOWN) {
                if (lastGuiMessage != RIGHT_KEY_DOWN) {
                    direction = TurnDirection::LEFT;
                    lastGuiMessage = guiMessage;
                }
            }
        } catch (...) {
            std::cerr << "[GUI] " << " GUI MESSAGE NOT VALID " << std::endl;
        }
    }

    void handleGameOver() {
        nextExpectedEventNo = 0;
        isInGame = false;
    }

    void handlePlayerEliminated(Record &record) {
        std::shared_ptr<PlayerEliminatedData> playerEliminatedData = (const std::shared_ptr<PlayerEliminatedData> &) record.getEventData();
        std::string playerStr = "PLAYER ELIMINATED";
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
        if (eventNo != nextExpectedEventNo)
            return;
        nextExpectedEventNo++;
        if (eventType == ServerEventType::GAME_OVER) {
            handleGameOver();
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
    ClientSide(const ClientData &clientData) : data(clientData),
                                               guiSock(data.getGuiPortNum(), data.getGuiAddress().c_str()),
                                               sock(data.getServerPortNum(), data.getServerAddress().c_str()),
                                               pollClient(sock.getSocket(), guiSock.getSocket()),
                                               lastGuiMessage(NO_MSG), direction(TurnDirection::STRAIGHT), isInGame(false), sessionID(std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::system_clock::now().time_since_epoch()
    ).count()) {
        std::cout << "constructed clientside!\n";
    }

//    ClientSide(const ClientData &clientData) : data(clientData),
//                                               sock(data.getServerPortNum(), data.getServerAddress().c_str()),
//
//                                               pollClient(sock.getSocket(), 0) {
//
//    }


    void sendLineToGUI(std::string line) {
        std::string lineWithNewLine = line + "\n";
        send(pollClient.getDescriptor(PollClient::MESSAGE_GUI), lineWithNewLine.c_str(), lineWithNewLine.size(), 0);
    }

    void tryToReadNewGame(ReadPacket &packet) {
        debug_out_0 << "Received new Game " << std::endl;
        Record record = Record::decode(packet);
        if (record.getEventType() != ServerEventType::NEW_GAME) {
            return;
        }
        if (record.getEventNo() != 0) {
            debug_out_1 << "New game event no != 0" << std::endl;
            return;
        }
        debug_out_0 << "New game comunicate parsing " << std::endl;
        std::shared_ptr<NewGameData> newGamePtr = (const std::shared_ptr<NewGameData> &) record.getEventData();
        debug_out_0 << "New game comunicate correct " << std::endl;
        std::cout << *newGamePtr << std::endl;
        std::string newGameString = "NEW GAME";
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
                std::cout << "MESSAGE GUI " << std::endl;
                std::string guiMsgStr;
                if (getline(guiInputStream, guiMsgStr)) {
                    std::cout << "[GUI] " << "read " << guiMsgStr << " from GUI!" << std::endl;
                    handleGuiMessage(guiMsgStr);
                } else {
                    std::cerr << "Invalid read from GUI " << std::endl;
                }
            }
            if (pollClient.hasPollinOccurred(PollClient::MESSAGE_SERVER)) {
                std::cout << "Message server!" << std::endl;
                ReadPacket readPacket;
                sockaddr_storage sockaddrStorage;
                socklen_t addrLen;
                recvfrom(pollClient.getDescriptor(PollClient::MESSAGE_SERVER), readPacket.getBuffer(),
                         readPacket.getMaxSize(), 0, reinterpret_cast<sockaddr *>(&sockaddrStorage),
                         &addrLen);
                uint32_t rcvdGameID = ServerMessage::getGameId(readPacket);
                std::cout << "received game id = " << rcvdGameID << std::endl;
                if (isInGame) {
                    if (rcvdGameID == currGameId) {
                        try {
                            while (1) {
                                Record record = Record::decode(readPacket);
                                handleGameRecord(record);
                            }
                        } catch (...) {
                            std::cerr << "Error / END parsing event record";
                        }
                    }
                } else {
                    try {
                        tryToReadNewGame(readPacket);
                        while (1) {
                            Record record = Record::decode(readPacket);
                            handleGameRecord(record);
                        }
                    } catch (...) {
                        std::cerr << "Error reading newGame " << std::endl;
                    }
                }
            }
            if (pollClient.hasPollinOccurred(PollClient::INTERVAL_SENDMESSAGE)) {
                std::cout << "Sending message to server!" << std::endl;
                ClientMessage clientMessage(sessionID, static_cast<uint8_t>(direction), nextExpectedEventNo, data.getPlayerName().c_str(), data.getPlayerName().size());
                std::cout << clientMessage << std::endl;
                WritePacket writePacket;
                clientMessage.encode(writePacket);
                sendto(sock.getSocket(), writePacket.getBufferConst(), writePacket.getOffset(), 0,
                       sock.getAddrInfo().ai_addr, sock.getAddrInfo().ai_addrlen);
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